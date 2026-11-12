#pragma once
#include "software_benchmark.h"
#include <unistd.h>
#include "content_services.h"
namespace probe {
class PcmQueue {
public:
    using Sink=bool(*)(void*,const int16_t*);
private:
    pthread_mutex_t mutex_=PTHREAD_MUTEX_INITIALIZER;pthread_cond_t changed_=PTHREAD_COND_INITIALIZER;
    pthread_t thread_{};bool started_=false,stop_=false,failed_=false,ending_=false,paused_=false;double pauseStart_=0,pauseTotal_=0;
    alignas(16) int16_t blocks_[16][2048]{};unsigned read_=0,count_=0,partial_=0;uint64_t submitted_=0,underruns_=0;
    int16_t pending_[2048]{};double first_=0,mediaOrigin_=0;bool haveMediaOrigin_=false;uint64_t pauseEpoch_=0;Sink sink_;void* opaque_;BenchLog log_;
    std::vector<MuteRange> muteRanges_;
    static void* entry(void* p) {static_cast<PcmQueue*>(p)->work();return nullptr;}
    void work() {
        for(;;) {
            alignas(16) int16_t block[2048];pthread_mutex_lock(&mutex_);
            double emptyAt=first_ && !count_ && !paused_?benchmarkTime():0;uint64_t epoch=pauseEpoch_;
            while((!count_ || paused_) && !stop_ && !ending_) {
                pthread_cond_wait(&changed_,&mutex_);
                if(epoch!=pauseEpoch_) {emptyAt=0;epoch=pauseEpoch_;}
                if(!emptyAt && first_ && !count_ && !paused_) emptyAt=benchmarkTime();
            }
            if(emptyAt && epoch==pauseEpoch_ && count_ && !stop_) {double gap=benchmarkTime()-emptyAt;if(gap>0.025) {pauseTotal_+=gap;++underruns_;}}
            if(stop_ || (!count_ && ending_)) {pthread_mutex_unlock(&mutex_);break;}
            memcpy(block,blocks_[read_],sizeof(block));read_=(read_+1)%16;--count_;
            if(!first_) first_=benchmarkTime();
            if(haveMediaOrigin_ && !muteRanges_.empty()) muteSponsorSamples(block,1024,mediaOrigin_+submitted_/48000.0,muteRanges_);
            pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);
            bool ok=sink_(opaque_,block);
            pthread_mutex_lock(&mutex_);if(ok) submitted_+=1024;else {failed_=true;stop_=true;}pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);
            if(!ok) break;
        }
    }
    bool putBlock() {
        pthread_mutex_lock(&mutex_);while(count_==16 && !stop_) pthread_cond_wait(&changed_,&mutex_);
        if(stop_) {pthread_mutex_unlock(&mutex_);return false;}
        memcpy(blocks_[(read_+count_)%16],pending_,sizeof(pending_));++count_;pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);partial_=0;return true;
    }
public:
    PcmQueue(Sink sink,void* opaque,BenchLog log):sink_(sink),opaque_(opaque),log_(log) {}
    bool start() {
        pthread_attr_t attr;int rc=pthread_attr_init(&attr);if(rc) return false;
        rc=pthread_attr_setstacksize(&attr,2*1024*1024);if(!rc) rc=pthread_create(&thread_,&attr,entry,this);pthread_attr_destroy(&attr);started_=rc==0;return started_;
    }
    bool write(const int16_t* pcm,unsigned frames) {
        while(frames) {unsigned n=frames<1024-partial_?frames:1024-partial_;memcpy(pending_+partial_*2,pcm,n*4);pcm+=n*2;frames-=n;partial_+=n;if(partial_==1024 && !putBlock()) return false;}return true;
    }
    void muteRanges(std::vector<MuteRange> ranges) {pthread_mutex_lock(&mutex_);muteRanges_=std::move(ranges);pthread_mutex_unlock(&mutex_);}
    void mediaOrigin(double pts) {pthread_mutex_lock(&mutex_);if(!haveMediaOrigin_) {mediaOrigin_=pts;haveMediaOrigin_=true;}pthread_mutex_unlock(&mutex_);}
    bool getMediaOrigin(double& pts) {pthread_mutex_lock(&mutex_);pts=mediaOrigin_;bool valid=haveMediaOrigin_;pthread_mutex_unlock(&mutex_);return valid;}
    double firstOutput() {pthread_mutex_lock(&mutex_);double result=first_?first_+pauseTotal_:0;pthread_mutex_unlock(&mutex_);return result;}
    void pause(bool value) {pthread_mutex_lock(&mutex_);if(value && !paused_) ++pauseEpoch_;if(value && !paused_) pauseStart_=first_?benchmarkTime():0;if(!value && paused_ && pauseStart_) pauseTotal_+=benchmarkTime()-pauseStart_;paused_=value;pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);}
    void fail() {pthread_mutex_lock(&mutex_);failed_=true;stop_=true;pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);}
    bool failed() {pthread_mutex_lock(&mutex_);bool result=failed_;pthread_mutex_unlock(&mutex_);return result;}
    void finish(bool drain) {
        if(drain && partial_) {memset(pending_+partial_*2,0,(1024-partial_)*4);putBlock();}
        pthread_mutex_lock(&mutex_);ending_=true;if(!drain) stop_=true;pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);
        if(started_) {pthread_join(thread_,nullptr);started_=false;}
        log_("PCM result submitted_frames=%llu queue_capacity_blocks=16 block_frames=1024 failed=%d rebuffer_events=%llu",(unsigned long long)submitted_,failed_,(unsigned long long)underruns_);
    }
};
}
