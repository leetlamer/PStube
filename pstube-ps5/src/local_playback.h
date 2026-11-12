#pragma once
#include "software_benchmark.h"
#include <unistd.h>
extern "C" {
#include <libswscale/swscale.h>
}
namespace probe {
inline int fitWidth(int width,int height,int outputWidth=1920,int outputHeight=1080) {return width*outputHeight<height*outputWidth?((width*outputHeight/height)&~1):outputWidth;}
inline int fitHeight(int width,int height,int outputWidth=1920,int outputHeight=1080) {return width*outputHeight>height*outputWidth?((height*outputWidth/width)&~1):outputHeight;}
// Display and input stay on the owning main thread. FFmpeg owns decoder threads.
using PresentPicture=bool(*)(void*,const AVFrame*,SwsContext*,unsigned,int,unsigned,double*,double*);
struct PlaybackTiming {
    double samples[1200]{};unsigned count=0,late=0;double decode=0,convert=0,present=0;
    void add(double ms) {if(count<1200) samples[count++]=ms;}
    double percentile(unsigned percent) {
        double sorted[1200];memcpy(sorted,samples,count*sizeof(double));
        for(unsigned i=1;i<count;++i) {double v=sorted[i];unsigned j=i;while(j && sorted[j-1]>v) {sorted[j]=sorted[j-1];--j;}sorted[j]=v;}
        return count?sorted[(count-1)*percent/100]:0;
    }
};
inline bool playLocal(unsigned height,int threads,PresentPicture present,void* context,BenchLog log) {
    char path[256];snprintf(path,sizeof(path),"/data/homebrew/PStube-PS5-Test/assets/vp9-%u.ivf",height);
    FILE* f=fopen(path,"rb");if(!f) {snprintf(path,sizeof(path),"assets/vp9-%u.ivf",height);f=fopen(path,"rb");}
    if(!f) {log("PLAY missing_fixture height=%u",height);return false;}
    if(fseek(f,0,SEEK_END)) {fclose(f);return false;}long bytes=ftell(f);rewind(f);
    if(bytes<32 || bytes>16*1024*1024) {fclose(f);return false;}
    auto* data=static_cast<uint8_t*>(malloc((size_t)bytes));bool read=data && fread(data,1,(size_t)bytes,f)==(size_t)bytes;fclose(f);
    IvfView ivf;if(!read || !ivf.open(data,(size_t)bytes) || ivf.height!=height) {free(data);return false;}
    const AVCodec* codec=avcodec_find_decoder(AV_CODEC_ID_VP9);AVCodecContext* decoder=codec?avcodec_alloc_context3(codec):nullptr;
    AVFrame* picture=av_frame_alloc();AVPacket* packet=av_packet_alloc();SwsContext* scaler=nullptr;
    bool ok=decoder && picture && packet;PlaybackTiming timing;unsigned displayed=0;double start=0;
    if(ok) {decoder->thread_count=threads;decoder->thread_type=FF_THREAD_FRAME|FF_THREAD_SLICE;ok=avcodec_open2(decoder,codec,nullptr)>=0;}
    if(ok) scaler=sws_getContext(ivf.width,ivf.height,AV_PIX_FMT_YUV420P,1920,1080,AV_PIX_FMT_RGBA,SWS_BILINEAR,nullptr,nullptr,nullptr);
    ok=ok && scaler;log("PLAY begin source_height=%u output=1920x1080 threads=%d repeats=5 source_fps=60 scale=bilinear bounded_decoder_frames=1 no_audio_network=1 pacing=display_flip no_inter_repeat_flush=1",height,threads);
    // Default fixture testsrc2 is limited-range BT.601; explicitly select matching conversion.
    if(ok) {const int* coefficients=sws_getCoefficients(SWS_CS_ITU601);ok=sws_setColorspaceDetails(scaler,coefficients,0,coefficients,1,0,1<<16,1<<16)>=0;}
    auto drain=[&]() {
        for(;;) {
            double before=benchmarkTime();int rc=avcodec_receive_frame(decoder,picture);timing.decode+=(benchmarkTime()-before)*1000;
            if(rc==AVERROR(EAGAIN)||rc==AVERROR_EOF) return true;
            if(rc<0 || picture->width!=(int)ivf.width || picture->height!=(int)ivf.height || picture->format!=AV_PIX_FMT_YUV420P) {log("PLAY receive_error=%d",rc);return false;}
            double conversion=0,presentation=0;
            if(!start) start=benchmarkTime();
            if(benchmarkTime()-start>45) {av_frame_unref(picture);log("PLAY timeout_seconds=45");return false;}
            double begin=benchmarkTime();bool shown=present(context,picture,scaler,height,threads,displayed,&conversion,&presentation);
            timing.add((benchmarkTime()-begin)*1000);timing.convert+=conversion;timing.present+=presentation;
            av_frame_unref(picture);if(!shown) return false;++displayed;
        }
    };
    for(unsigned repeat=0;ok && repeat<5;++repeat) {
        log("PLAY input_boundary source_height=%u threads=%d repeat=%u displayed=%u",height,threads,repeat,displayed);
        ivf.open(data,(size_t)bytes);const uint8_t* payload=nullptr;size_t length=0;
        while(ok && ivf.next(payload,length)) {
            int rc=av_new_packet(packet,(int)length);if(rc<0) {ok=false;break;}memcpy(packet->data,payload,length);
            double before=benchmarkTime();rc=avcodec_send_packet(decoder,packet);timing.decode+=(benchmarkTime()-before)*1000;av_packet_unref(packet);
            if(rc<0) {log("PLAY send_error=%d",rc);ok=false;break;}ok=drain();
        }
    }
    if(ok) {double before=benchmarkTime();int rc=avcodec_send_packet(decoder,nullptr);timing.decode+=(benchmarkTime()-before)*1000;ok=rc>=0 && drain();}
    double elapsed=start?benchmarkTime()-start:0;
    log("PLAY result source_height=%u threads=%d complete=%d displayed=%u expected=600 legacy_late_counter_unused=%u elapsed_ms=%.3f presentation_fps=%.3f decode_api_total_ms=%.3f convert_average_ms=%.3f tile_flip_wait_average_ms=%.3f convert_present_p50_ms=%.3f p95_ms=%.3f p99_ms=%.3f output_height=1080 dropped=0 drop_policy=none",height,threads,ok&&displayed==600,displayed,timing.late,elapsed*1000,elapsed>0?displayed/elapsed:0,timing.decode,displayed?timing.convert/displayed:0,displayed?timing.present/displayed:0,timing.percentile(50),timing.percentile(95),timing.percentile(99));
    sws_freeContext(scaler);av_packet_free(&packet);av_frame_free(&picture);avcodec_free_context(&decoder);free(data);return ok && displayed==600;
}
}
