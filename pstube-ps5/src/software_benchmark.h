#pragma once
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ivf.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/cpu.h>
#include <libavutil/error.h>
}
namespace probe {
using BenchLog=void(*)(const char*,...);
inline double benchmarkTime() {timespec t{};if(clock_gettime(CLOCK_MONOTONIC,&t)) return 0;return t.tv_sec+t.tv_nsec/1e9;}
struct BenchmarkStatus {bool running=false,done=false,cancelled=false;unsigned frames=0,test=0;double fps=0;char line[96]="CROSS STARTS VP NINE BENCHMARK";};
class SoftwareBenchmark {
    pthread_mutex_t mutex_=PTHREAD_MUTEX_INITIALIZER;pthread_t thread_{};bool started_=false,cancel_=false;
    BenchmarkStatus state_{};BenchLog log_;
    bool stopped() {pthread_mutex_lock(&mutex_);bool result=cancel_;pthread_mutex_unlock(&mutex_);return result;}
    void status(const char* text,unsigned frames=0,double fps=0) {
        pthread_mutex_lock(&mutex_);snprintf(state_.line,sizeof(state_.line),"%s",text);state_.frames=frames;state_.fps=fps;pthread_mutex_unlock(&mutex_);
    }
    bool receive(AVCodecContext* ctx,AVFrame* frame,unsigned& decoded,unsigned width,unsigned height) {
        for(;;) {
            int rc=avcodec_receive_frame(ctx,frame);
            if(rc==AVERROR(EAGAIN) || rc==AVERROR_EOF) return true;
            if(rc<0) {log_("SWBENCH receive error=%d",rc);return false;}
            if(frame->width!=(int)width || frame->height!=(int)height || frame->format!=AV_PIX_FMT_YUV420P || !frame->data[0]) {log_("SWBENCH invalid picture width=%d height=%d format=%d",frame->width,frame->height,frame->format);av_frame_unref(frame);return false;}
            ++decoded;av_frame_unref(frame);
            if(stopped()) return false;
        }
    }
    bool run(unsigned height,int threads,unsigned index) {
        char relative[80],path[512];snprintf(relative,sizeof(relative),"assets/vp9-%u.ivf",height);
        FILE* file=fopen(relative,"rb");
        if(!file) {snprintf(path,sizeof(path),"/data/homebrew/PStube-PS5-Test/%s",relative);file=fopen(path,"rb");}
        if(!file) {log_("SWBENCH missing_fixture=%s",relative);status("FIXTURE FILE MISSING SEE LOG");return false;}
        if(fseek(file,0,SEEK_END)) {fclose(file);return false;}long bytes=ftell(file);rewind(file);
        if(bytes<32 || bytes>16*1024*1024) {fclose(file);return false;}
        auto* data=static_cast<uint8_t*>(malloc((size_t)bytes));if(!data) {fclose(file);return false;}
        bool read=fread(data,1,(size_t)bytes,file)==(size_t)bytes;fclose(file);IvfView ivf;
        if(!read || !ivf.open(data,(size_t)bytes) || ivf.height!=height) {log_("SWBENCH invalid_fixture=%s",relative);free(data);return false;}
        const AVCodec* codec=avcodec_find_decoder(AV_CODEC_ID_VP9);
        AVCodecContext* ctx=codec?avcodec_alloc_context3(codec):nullptr;
        AVFrame* frame=av_frame_alloc();AVPacket* packet=av_packet_alloc();bool ok=ctx && frame && packet;
        double opened=benchmarkTime();
        if(ok) {ctx->thread_count=threads;ctx->thread_type=FF_THREAD_FRAME|FF_THREAD_SLICE;int rc=avcodec_open2(ctx,codec,nullptr);ok=rc>=0;if(!ok) log_("SWBENCH open error=%d",rc);}
        log_("SWBENCH begin test=%u source=synthetic_testsrc2 codec=vp9 height=%u requested_threads=%d active_threads=%d thread_type=%d open_ms=%.3f packet_count=%u excludes_network_audio_scaling=1",index,height,threads,ctx?ctx->thread_count:0,ctx?ctx->active_thread_type:0,(benchmarkTime()-opened)*1000,ivf.count);
        pthread_mutex_lock(&mutex_);state_.test=index;pthread_mutex_unlock(&mutex_);
        char label[96];snprintf(label,sizeof(label),"VP NINE %s %s",height==1080?"FULL HD":height==1440?"TWO K":"FOUR K",threads==4?"FOUR THREADS":"SIX THREADS");status(label);
        unsigned decoded=0,sent=0;double start=benchmarkTime(),end=start;const uint8_t* payload;size_t length;
        while(ok && !stopped() && ivf.next(payload,length)) {
            if(benchmarkTime()-start>45) {log_("SWBENCH workload timeout_seconds=45");ok=false;break;}
            int rc=av_new_packet(packet,(int)length);
            if(rc<0) {ok=false;break;}memcpy(packet->data,payload,length);
            rc=avcodec_send_packet(ctx,packet);av_packet_unref(packet);
            if(rc<0) {log_("SWBENCH send error=%d",rc);ok=false;break;}++sent;
            ok=receive(ctx,frame,decoded,ivf.width,ivf.height);end=benchmarkTime();
            status(label,decoded,end>start?decoded/(end-start):0);
        }
        if(ok && !stopped()) {int rc=avcodec_send_packet(ctx,nullptr);ok=rc>=0 && receive(ctx,frame,decoded,ivf.width,ivf.height);}
        end=benchmarkTime();double elapsed=end-start;bool cancelled=stopped();
        bool complete=ok && !cancelled && sent==ivf.count && decoded==ivf.count && elapsed>0;
        log_("SWBENCH result height=%u threads=%d complete=%d cancelled=%d packets=%u frames=%u elapsed_ms=%.3f throughput_fps=%.3f average_frame_ms=%.3f decode_only=1 paced_playback_tested=0",height,threads,complete,cancelled,sent,decoded,elapsed*1000,elapsed>0?decoded/elapsed:0,decoded?elapsed*1000/decoded:0);
        av_packet_free(&packet);av_frame_free(&frame);avcodec_free_context(&ctx);free(data);
        return complete;
    }
    void work() {
        log_("SWBENCH engine=%s cpu_flags=0x%x logical_cpus=%d synthetic_in_memory_decode_only=1",av_version_info(),av_get_cpu_flags(),av_cpu_count());
        unsigned index=0,passed=0;
        const unsigned heights[]={1080,1440,2160};const int threadCounts[]={4,6};
        for(unsigned height:heights) for(int threads:threadCounts) {
            if(stopped()) break;if(run(height,threads,++index)) ++passed;
        }
        bool cancelled=stopped();log_("SWBENCH series_end completed_workloads=%u attempted=%u cancelled=%d",passed,index,cancelled);
        pthread_mutex_lock(&mutex_);state_.running=false;state_.done=true;state_.cancelled=cancelled;
        snprintf(state_.line,sizeof(state_.line),"%s",cancelled?"BENCHMARK CANCELLED":passed==6?"BENCHMARK COMPLETE SEE LOG":"BENCHMARK INCOMPLETE SEE LOG");pthread_mutex_unlock(&mutex_);
    }
    static void* entry(void* context) {static_cast<SoftwareBenchmark*>(context)->work();return nullptr;}
public:
    explicit SoftwareBenchmark(BenchLog logger):log_(logger) {}
    BenchmarkStatus snapshot() {pthread_mutex_lock(&mutex_);auto result=state_;pthread_mutex_unlock(&mutex_);return result;}
    bool start() {
        if(started_) return false;
        pthread_attr_t attributes;int rc=pthread_attr_init(&attributes);if(rc) return false;
        rc=pthread_attr_setstacksize(&attributes,2*1024*1024);
        if(!rc) {pthread_mutex_lock(&mutex_);state_.running=true;pthread_mutex_unlock(&mutex_);rc=pthread_create(&thread_,&attributes,entry,this);}
        pthread_attr_destroy(&attributes);if(rc) {pthread_mutex_lock(&mutex_);state_.running=false;pthread_mutex_unlock(&mutex_);log_("SWBENCH thread_start error=%d",rc);return false;}
        started_=true;return true;
    }
    void cancelAndJoin() {pthread_mutex_lock(&mutex_);cancel_=true;pthread_mutex_unlock(&mutex_);if(started_) {int rc=pthread_join(thread_,nullptr);log_("SWBENCH worker_join rc=%d",rc);started_=false;}}
};
}
