#pragma once
#include <atomic>
namespace probe {
class AdaptiveAudio {
    AVIOContext* io_;PcmQueue& pcm_;BenchLog log_;pthread_t thread_{};bool started_=false;double startSeconds_=0;std::atomic<bool> stop_{false};
    static void* entry(void* p) {static_cast<AdaptiveAudio*>(p)->work();return nullptr;}
    void work() {
        AVFormatContext* input=avformat_alloc_context();AVCodecContext* decoder=nullptr;SwrContext* resampler=nullptr;AVPacket* packet=av_packet_alloc();AVFrame* frame=av_frame_alloc();
        bool ok=input && packet && frame;int rc=-1,track=-1;unsigned samples=0;
        if(ok) {input->pb=io_;input->flags|=AVFMT_FLAG_CUSTOM_IO;rc=avformat_open_input(&input,nullptr,nullptr,nullptr);ok=rc>=0;}
        if(ok) {rc=avformat_find_stream_info(input,nullptr);ok=rc>=0;}
        if(ok) {track=av_find_best_stream(input,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0);decoder=openTrack(input,track,log_);ok=decoder!=nullptr;}
        if(ok) {AVChannelLayout stereo=AV_CHANNEL_LAYOUT_STEREO;rc=swr_alloc_set_opts2(&resampler,&stereo,AV_SAMPLE_FMT_S16,48000,&decoder->ch_layout,decoder->sample_fmt,decoder->sample_rate,0,nullptr);ok=rc>=0 && swr_init(resampler)>=0;}
        if(ok && startSeconds_>0) {int64_t target=(int64_t)(startSeconds_*AV_TIME_BASE);rc=avformat_seek_file(input,-1,INT64_MIN,target,target,AVSEEK_FLAG_BACKWARD);log_("SEEK audio target=%.3f rc=%d",startSeconds_,rc);ok=rc>=0;}
        auto drain=[&]() {
            for(;;) {
                int got=avcodec_receive_frame(decoder,frame);if(got==AVERROR(EAGAIN)||got==AVERROR_EOF) return true;
                if(got<0 || stop_) return false;
                double pts=frame->best_effort_timestamp==AV_NOPTS_VALUE?0:frame->best_effort_timestamp*av_q2d(input->streams[track]->time_base);
                alignas(16) int16_t converted[16384];uint8_t* out[]={reinterpret_cast<uint8_t*>(converted)};
                int n=swr_convert(resampler,out,8192,const_cast<const uint8_t**>(frame->extended_data),frame->nb_samples);av_frame_unref(frame);
                int trim=startSeconds_>0?std::min(std::max(n,0),(int)std::min(8192.0,std::max(0.0,std::ceil((startSeconds_-pts)*48000)))):0;
                if(n>trim) pcm_.mediaOrigin(pts+trim/48000.0);
                if(n<0 || !pcm_.write(converted+trim*2,(unsigned)(n-trim))) return false;samples+=(unsigned)n;
            }
        };
        log_("ADAPTIVE audio_open ok=%d track=%d sample_rate=%d",ok,track,decoder?decoder->sample_rate:0);
        while(ok && !stop_ && (rc=av_read_frame(input,packet))>=0) {
            if(packet->stream_index==track) {int sent=avcodec_send_packet(decoder,packet);ok=sent>=0 && drain();}av_packet_unref(packet);
        }
        if(ok && !stop_ && rc==AVERROR_EOF) ok=avcodec_send_packet(decoder,nullptr)>=0 && drain();
        else if(!stop_) ok=false;
        if(!ok && !stop_) pcm_.fail();
        log_("ADAPTIVE audio_end ok=%d cancelled=%d samples=%u rc=%d",ok,stop_.load(),samples,rc);
        swr_free(&resampler);avcodec_free_context(&decoder);av_packet_free(&packet);av_frame_free(&frame);avformat_close_input(&input);
    }
public:
    AdaptiveAudio(AVIOContext* io,PcmQueue& pcm,BenchLog log,double startSeconds=0):io_(io),pcm_(pcm),log_(log),startSeconds_(startSeconds) {}
    bool start() {pthread_attr_t attr;int rc=pthread_attr_init(&attr);if(rc) return false;rc=pthread_attr_setstacksize(&attr,2*1024*1024);if(!rc) rc=pthread_create(&thread_,&attr,entry,this);pthread_attr_destroy(&attr);started_=rc==0;return started_;}
    void cancel() {stop_=true;}
    void join() {if(started_) {pthread_join(thread_,nullptr);started_=false;}}
};
}
