#pragma once
#include "tile.h"
#include <pthread.h>

// The display thread owns submissions; one persistent worker tiles the lower
// half. Row boundaries are multiples of four to avoid shared output cache lines.
class ParallelTiler {
    pthread_mutex_t mutex_=PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t changed_=PTHREAD_COND_INITIALIZER;
    pthread_t thread_{};
    bool started_=false,stop_=false,pending_=false,done_=true;
    const uint32_t* source_=nullptr;
    uint32_t* destination_=nullptr;
    unsigned width_=0,height_=0,split_=0;
    static void* entry(void* context) {
        static_cast<ParallelTiler*>(context)->work();return nullptr;
    }
    void work() {
        pthread_mutex_lock(&mutex_);
        for(;;) {
            while(!pending_ && !stop_) pthread_cond_wait(&changed_,&mutex_);
            if(stop_) break;
            const uint32_t* source=source_;uint32_t* destination=destination_;
            unsigned width=width_,height=height_,split=split_;
            pending_=false;pthread_mutex_unlock(&mutex_);
            tileFrameRows(source,destination,width,split,height);
            pthread_mutex_lock(&mutex_);done_=true;pthread_cond_broadcast(&changed_);
        }
        pthread_mutex_unlock(&mutex_);
    }
public:
    ParallelTiler()=default;
    ParallelTiler(const ParallelTiler&)=delete;
    ParallelTiler& operator=(const ParallelTiler&)=delete;
    ~ParallelTiler() {close();pthread_cond_destroy(&changed_);pthread_mutex_destroy(&mutex_);}
    bool start() {
        if(started_) return true;
        stop_=false;pending_=false;done_=true;
        pthread_attr_t attr;int rc=pthread_attr_init(&attr);if(rc) return false;
        rc=pthread_attr_setstacksize(&attr,2*1024*1024);
        if(!rc) rc=pthread_create(&thread_,&attr,entry,this);
        pthread_attr_destroy(&attr);started_=rc==0;return started_;
    }
    void frame(const uint32_t* source,uint32_t* destination,unsigned width,unsigned height) {
        if(!started_ || height<8) {tileFrameFast(source,destination,width,height);return;}
        unsigned split=(height/2)&~3u;
        pthread_mutex_lock(&mutex_);source_=source;destination_=destination;
        width_=width;height_=height;split_=split;done_=false;pending_=true;
        pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);
        tileFrameRows(source,destination,width,0,split);
        pthread_mutex_lock(&mutex_);while(!done_) pthread_cond_wait(&changed_,&mutex_);pthread_mutex_unlock(&mutex_);
    }
    void close() {
        if(!started_) return;
        pthread_mutex_lock(&mutex_);stop_=true;pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);
        pthread_join(thread_,nullptr);started_=false;
    }
};
