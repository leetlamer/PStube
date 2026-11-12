#pragma once
#include <algorithm>
#include "local_playback.h"
#include "pcm_queue.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}
namespace probe {
struct VideoStreamMetrics {int width=0,height=0,threads=0;double fps=0;const char* codec="unknown";};
inline int avFileRead(void* file,uint8_t* data,int size) {size_t got=fread(data,1,(size_t)size,static_cast<FILE*>(file));return got?(int)got:feof(static_cast<FILE*>(file))?AVERROR_EOF:AVERROR(EIO);}
inline int64_t avFileSeek(void* opaque,int64_t offset,int origin) {FILE* file=static_cast<FILE*>(opaque);if(origin==AVSEEK_SIZE) {off_t before=ftello(file);if(fseeko(file,0,SEEK_END)) return -1;off_t size=ftello(file);fseeko(file,before,SEEK_SET);return size;}origin &= ~AVSEEK_FORCE;if(origin!=SEEK_SET && origin!=SEEK_CUR && origin!=SEEK_END) return -1;return fseeko(file,offset,origin)?-1:ftello(file);}

inline AVCodecContext* openTrack(AVFormatContext* input,int track,BenchLog log,int threads=8) {
    if(track<0) return nullptr;auto* parameters=input->streams[track]->codecpar;
    const AVCodec* codec=avcodec_find_decoder(parameters->codec_id);AVCodecContext* decoder=codec?avcodec_alloc_context3(codec):nullptr;
    if(!decoder) return nullptr;decoder->thread_count=parameters->codec_type==AVMEDIA_TYPE_VIDEO?std::max(1,std::min(threads,10)):1;decoder->thread_type=FF_THREAD_FRAME|FF_THREAD_SLICE;
    int rc=avcodec_parameters_to_context(decoder,parameters);if(rc>=0) rc=avcodec_open2(decoder,codec,nullptr);
    if(rc<0) {log("AV open_decoder track=%d rc=%d",track,rc);avcodec_free_context(&decoder);}
    else log("AV decoder track=%d codec=%s threads=%d active_thread_type=%d",track,avcodec_get_name(decoder->codec_id),decoder->thread_count,decoder->active_thread_type);return decoder;
}
inline bool playAv(const char* path,PresentPicture present,void* display,PcmQueue& pcm,BenchLog log,AVIOContext* external=nullptr,unsigned maxPictures=0,bool videoOnly=false,int outputWidth=1920,int outputHeight=1080,bool recoverLate=false,bool liveTimeline=false,double* mediaPosition=nullptr,double startSeconds=0,VideoStreamMetrics* metrics=nullptr,int decoderThreads=8,int scalerThreads=2) {
    AVFormatContext* input=nullptr;AVCodecContext* video=nullptr;AVCodecContext* audio=nullptr;SwsContext* scaler=nullptr;SwrContext* resampler=nullptr;
    AVPacket* packet=av_packet_alloc();AVFrame* frame=av_frame_alloc();bool ok=packet && frame;unsigned pictures=0,audioFrames=0,late=0,decodedPictures=0,droppedPictures=0;bool limited=false;double anchor=0,maxSkew=0,firstPts=0;bool haveFirstPts=false;
    FILE* file=external?nullptr:fopen(path,"rb");AVIOContext* io=external;
    if(file) {auto* buffer=static_cast<uint8_t*>(av_malloc(65536));if(buffer) {io=avio_alloc_context(buffer,65536,0,file,avFileRead,nullptr,avFileSeek);if(!io) av_free(buffer);}}
    if(io) {input=avformat_alloc_context();if(input) {input->pb=io;input->flags|=AVFMT_FLAG_CUSTOM_IO;}}
    int rc=ok && input?avformat_open_input(&input,nullptr,nullptr,nullptr):-1;
    if(rc<0) log("AV open_input rc=%d file_open=%d io_allocated=%d",rc,file!=nullptr,io!=nullptr);
    if(rc>=0) rc=avformat_find_stream_info(input,nullptr);ok=rc>=0;
    int vi=ok?av_find_best_stream(input,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0):-1;
    int ai=ok && !videoOnly?av_find_best_stream(input,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0):-1;
    if(ok) {video=openTrack(input,vi,log,decoderThreads);audio=openTrack(input,ai,log);ok=video && (audio || videoOnly) && video->width>0 && video->width<=3840 && video->height>0 && video->height<=2160;}
    if(ok && startSeconds>0) {int64_t target=(int64_t)(startSeconds*AV_TIME_BASE);rc=avformat_seek_file(input,-1,INT64_MIN,target,target,AVSEEK_FLAG_BACKWARD);log("SEEK video target=%.3f rc=%d",startSeconds,rc);ok=rc>=0;}
    if(ok) {
        scaler=sws_alloc_context();
        if(scaler) {
            av_opt_set_int(scaler,"srcw",video->width,0);av_opt_set_int(scaler,"srch",video->height,0);av_opt_set_int(scaler,"src_format",video->pix_fmt,0);
            av_opt_set_int(scaler,"dstw",fitWidth(video->width,video->height,outputWidth,outputHeight),0);av_opt_set_int(scaler,"dsth",fitHeight(video->width,video->height,outputWidth,outputHeight),0);av_opt_set_int(scaler,"dst_format",AV_PIX_FMT_RGBA,0);
            av_opt_set_int(scaler,"sws_flags",SWS_BILINEAR,0);av_opt_set_int(scaler,"threads",std::max(1,std::min(scalerThreads,2)),0);
            if(sws_init_context(scaler,nullptr,nullptr)<0) {sws_freeContext(scaler);scaler=nullptr;}
        }
        log("AV resources decoder_threads=%d scaler_threads=%d online_cpus=%ld streaming_ring_bytes=%u HDR_output=0",video->thread_count,scalerThreads,sysconf(_SC_NPROCESSORS_ONLN),8u*1024u*1024u);
        AVChannelLayout stereo=AV_CHANNEL_LAYOUT_STEREO;
        rc=0;if(audio) rc=swr_alloc_set_opts2(&resampler,&stereo,AV_SAMPLE_FMT_S16,48000,&audio->ch_layout,audio->sample_fmt,audio->sample_rate,0,nullptr);
        ok=scaler && rc>=0 && (!audio || swr_init(resampler)>=0);
    }
    log("AV output width=%d height=%d",outputWidth,outputHeight);
    log("AV begin opened=%d video_track=%d audio_track=%d width=%d height=%d sample_rate=%d audio_master=estimated_monotonic_first_submit",ok,vi,ai,video?video->width:0,video?video->height:0,audio?audio->sample_rate:0);
    double fallbackFps=30.0,lastVideoPts=0;bool haveVideoPts=false;
    if(ok) {AVRational rate=av_guess_frame_rate(input,input->streams[vi],nullptr);double fps=av_q2d(rate);if(std::isfinite(fps) && fps>=1 && fps<=240) fallbackFps=fps;}
    if(ok && metrics) {*metrics={video->width,video->height,video->thread_count,fallbackFps,avcodec_get_name(video->codec_id)};}
    log("CADENCE fallback_fps=%.6f valid_pts_priority=1 hdmi_mode_switch=0",fallbackFps);
    auto drain=[&](AVCodecContext* decoder,bool isAudio) {
        for(;;) {
            int got=avcodec_receive_frame(decoder,frame);if(got==AVERROR(EAGAIN)||got==AVERROR_EOF) return true;
            if(got<0 || pcm.failed()) {log("AV receive_failed rc=%d pcm_failed=%d",got,pcm.failed());return false;}
            if(isAudio) {
                double audioPts=frame->best_effort_timestamp==AV_NOPTS_VALUE?0:frame->best_effort_timestamp*av_q2d(input->streams[ai]->time_base);
                alignas(16) int16_t converted[16384];uint8_t* out[]={reinterpret_cast<uint8_t*>(converted)};
                int frames=swr_convert(resampler,out,8192,const_cast<const uint8_t**>(frame->extended_data),frame->nb_samples);
                int trim=startSeconds>0?std::min(std::max(frames,0),(int)std::min(8192.0,std::max(0.0,std::ceil((startSeconds-audioPts)*48000)))):0;
                if(frames>trim) pcm.mediaOrigin(audioPts+trim/48000.0);
                if(frames<0 || !pcm.write(converted+trim*2,(unsigned)(frames-trim))) {av_frame_unref(frame);return false;}audioFrames+=(unsigned)frames;
            } else {
                if(frame->width!=video->width || frame->height!=video->height || frame->format!=video->pix_fmt) {av_frame_unref(frame);log("AV picture_layout_changed");return false;}
                double rawPts=frame->best_effort_timestamp==AV_NOPTS_VALUE?startSeconds:frame->best_effort_timestamp*av_q2d(input->streams[vi]->time_base);
                if(startSeconds>0 && rawPts+0.00001<startSeconds) {av_frame_unref(frame);continue;}
                if(videoOnly && !anchor) {pcm.pause(false);double wait=benchmarkTime();while(!pcm.firstOutput() && !pcm.failed() && benchmarkTime()-wait<10) usleep(1000);if(!pcm.firstOutput() || pcm.failed()) {av_frame_unref(frame);log("AV audio_start_failed");return false;}}
                double pts=frame->best_effort_timestamp==AV_NOPTS_VALUE?(haveVideoPts?lastVideoPts+1.0/fallbackFps:startSeconds):frame->best_effort_timestamp*av_q2d(input->streams[vi]->time_base);
                lastVideoPts=pts;haveVideoPts=true;
                if(liveTimeline) {if(!haveFirstPts) {firstPts=pts;double origin=0;if(pcm.getMediaOrigin(origin) && fabs(origin-pts)<10) firstPts=origin;haveFirstPts=true;log("AV live_origin video_minus_audio_ms=%.3f",(pts-firstPts)*1000);}pts-=firstPts;}
                ++decodedPictures;double audioStart=pcm.firstOutput();if(audioStart) anchor=audioStart-startSeconds;
                if(!anchor) anchor=benchmarkTime()-startSeconds;double target=anchor+pts;
                while(benchmarkTime()<target) usleep(1000);
                double skew=benchmarkTime()-target;if(skew>maxSkew) maxSkew=skew;if(skew>0.025) ++late;
                if(recoverLate && decodedPictures>4 && skew>0.035) {++droppedPictures;av_frame_unref(frame);continue;}
                if(mediaPosition) *mediaPosition=std::max(0.0,pts*1000);
                double conversion=0,presentation=0;
                if(!present(display,frame,scaler,(unsigned)video->height,video->thread_count,pictures,&conversion,&presentation)) {av_frame_unref(frame);return false;}
                ++pictures;if(maxPictures && pictures>=maxPictures) {limited=true;av_frame_unref(frame);return false;}
            }
            av_frame_unref(frame);
        }
    };
    while(ok && (rc=av_read_frame(input,packet))>=0) {
        AVCodecContext* decoder=packet->stream_index==vi?video:packet->stream_index==ai?audio:nullptr;
        if(decoder) {int result=avcodec_send_packet(decoder,packet);ok=result>=0 && drain(decoder,decoder==audio);if(result<0) log("AV packet_failed stream=%d rc=%d",packet->stream_index,result);}
        av_packet_unref(packet);
    }
    if(ok && rc!=AVERROR_EOF) {log("AV read_error=%d",rc);ok=false;}
    if(ok && audio) ok=avcodec_send_packet(audio,nullptr)>=0 && drain(audio,true);
    if(ok) ok=avcodec_send_packet(video,nullptr)>=0 && drain(video,false);
    log("AV result complete=%d limited=%d pictures=%u audio_frames=%u decoded_pictures=%u dropped_for_sync=%u video_late_over_25ms=%u max_estimated_video_lateness_ms=%.3f",ok,limited,pictures,audioFrames,decodedPictures,droppedPictures,late,maxSkew*1000);
    swr_free(&resampler);sws_freeContext(scaler);avcodec_free_context(&video);avcodec_free_context(&audio);av_packet_free(&packet);av_frame_free(&frame);avformat_close_input(&input);if(io && !external) {av_freep(&io->buffer);avio_context_free(&io);}if(file) fclose(file);return ok;
}
}
