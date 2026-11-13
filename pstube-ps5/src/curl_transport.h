#pragma once
#include "console_curl.h"
#include "buffer_policy.h"
#include <functional>
#include <unistd.h>
extern "C" {
#include <libavformat/avio.h>
}
#include "seek_timeline.h"
#include "software_benchmark.h"
namespace probe {
inline bool curlProbe(BenchLog log) {
    int init=curl_global_init(CURL_GLOBAL_DEFAULT);CURL* handle=init==0?curl_easy_init():nullptr;if(!handle) {log("CURL init_failed=%d",init);return false;}
    console_curl_setup(handle);curl_easy_setopt(handle,CURLOPT_URL,"https://www.youtube.com/generate_204");curl_easy_setopt(handle,CURLOPT_CONNECTTIMEOUT,15L);curl_easy_setopt(handle,CURLOPT_TIMEOUT,25L);
    curl_easy_setopt(handle,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(handle,CURLOPT_SSL_VERIFYHOST,2L);
    char error[CURL_ERROR_SIZE]{};curl_easy_setopt(handle,CURLOPT_ERRORBUFFER,error);
    CURLcode rc=curl_easy_perform(handle);long status=0;curl_easy_getinfo(handle,CURLINFO_RESPONSE_CODE,&status);
    log("CURL result ok=%d status=%ld rc=%d error=%s ca=%s tls_verify_peer=1 tls_verify_host=2 version=%s",rc==CURLE_OK && status==204,status,(int)rc,error,console_curl_ca_file(),curl_version());
    curl_easy_cleanup(handle);return rc==CURLE_OK && status==204;
}
}
#include "shared/core.h"
#include <atomic>
#include "shared/live_hls.h"
namespace probe {
struct Document {std::string body,error;long status=0;size_t limit=4*1024*1024;};
inline size_t documentWrite(char* bytes,size_t size,size_t count,void* opaque) {
    auto& result=*static_cast<Document*>(opaque);if(size && count>SIZE_MAX/size) return 0;size_t n=size*count;
    if(n>result.limit-result.body.size()) {result.error="Response exceeds limit";return 0;}result.body.append(bytes,n);return n;
}
inline Document performRequest(const smarttube::Request& request,size_t limit=4*1024*1024,long timeout=30,const std::atomic<bool>* cancelled=nullptr,CURL* reusable=nullptr,const console_network_settings* networkOverride=nullptr) {
    Document result;result.limit=limit;CURL* curl=reusable?reusable:curl_easy_init();if(reusable) curl_easy_reset(curl);if(!curl) {result.error="No HTTP context";return result;}
    console_curl_setup(curl);CURLcode network=console_curl_apply_network(curl,networkOverride);if(network!=CURLE_OK) {result.error="Network configuration unavailable ("+std::to_string((int)network)+")";if(!reusable) curl_easy_cleanup(curl);return result;}curl_easy_setopt(curl,CURLOPT_URL,request.url.c_str());curl_easy_setopt(curl,CURLOPT_USERAGENT,request.userAgent.c_str());
    curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,15L);curl_easy_setopt(curl,CURLOPT_TIMEOUT,timeout);curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(curl,CURLOPT_MAXREDIRS,3L);
    curl_easy_setopt(curl,CURLOPT_PROTOCOLS_STR,"https");curl_easy_setopt(curl,CURLOPT_REDIR_PROTOCOLS_STR,"https");
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,documentWrite);curl_easy_setopt(curl,CURLOPT_WRITEDATA,&result);
    curl_slist* headers=nullptr;headers=curl_slist_append(headers,("Content-Type: "+request.contentType).c_str());
    auto header=[&](const char* key,const std::string& value) {if(!value.empty()) headers=curl_slist_append(headers,(std::string(key)+": "+value).c_str());};
    header("Authorization",request.authorization);header("Accept",request.accept);
    header("X-YouTube-Client-Name",request.clientNumber);header("X-YouTube-Client-Version",request.clientVersion);header("X-Goog-Visitor-Id",request.visitorData);
    curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
    if(!request.httpGet) {curl_easy_setopt(curl,CURLOPT_POSTFIELDS,request.body.c_str());curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE_LARGE,(curl_off_t)request.body.size());}
    if(cancelled) {curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,+[](void* p,curl_off_t,curl_off_t,curl_off_t,curl_off_t)->int {return static_cast<const std::atomic<bool>*>(p)->load()?1:0;});curl_easy_setopt(curl,CURLOPT_XFERINFODATA,cancelled);}
    char error[CURL_ERROR_SIZE]{};curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,error);CURLcode rc=curl_easy_perform(curl);curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&result.status);
    if(rc!=CURLE_OK && result.error.empty()) result.error=error[0]?error:curl_easy_strerror(rc);
    if(result.status!=200 && result.error.empty()) result.error="HTTP "+std::to_string(result.status);
    curl_slist_free_all(headers);if(!reusable) curl_easy_cleanup(curl);return result;
}
class CurlStream {
    pthread_mutex_t mutex_=PTHREAD_MUTEX_INITIALIZER;pthread_cond_t changed_=PTHREAD_COND_INITIALIZER;pthread_t thread_{};
    size_t capacity_=8*1024*1024,startupBytes_=1,resumeBytes_=1;bool primed_=false;std::function<void(bool)> buffering_;uint8_t* data_=nullptr;size_t read_=0,count_=0;bool stop_=false,done_=false,started_=false,failed_=false;
    std::atomic<bool> cancelled_{false};bool hls_=false;
    int64_t offset_=0,length_=-1,rangeStart_=-1;long headerStatus_=0;bool headersReady_=false;
    std::string url_,agent_;uint64_t downloaded_=0,consumed_=0,faultAt_=0;bool faulted_=false;BenchLog log_;
    static size_t header(char* bytes,size_t size,size_t count,void* opaque) {
        if(size && count>SIZE_MAX/size) return 0;size_t n=size*count;auto& self=*static_cast<CurlStream*>(opaque);std::string line(bytes,n);
        pthread_mutex_lock(&self.mutex_);
        if(line.compare(0,5,"HTTP/")==0) {auto space=line.find(' ');self.headerStatus_=space==std::string::npos?0:strtol(line.c_str()+space+1,nullptr,10);self.rangeStart_=-1;self.headersReady_=false;}
        if(!strncasecmp(line.c_str(),"Content-Length:",15) && self.headerStatus_==200) self.length_=strtoll(line.c_str()+15,nullptr,10);
        if(!strncasecmp(line.c_str(),"Content-Range:",14)) {long long start=-1,end=-1,total=-1;if(sscanf(line.c_str()+14," bytes %lld-%lld/%lld",&start,&end,&total)==3 && start>=0 && end>=start && total>end) {self.rangeStart_=start;self.length_=total;}}
        if(line=="\r\n" && self.headerStatus_>=200 && self.headerStatus_<300) self.headersReady_=true;
        pthread_cond_broadcast(&self.changed_);pthread_mutex_unlock(&self.mutex_);return n;
    }
    static size_t write(char* bytes,size_t size,size_t count,void* opaque) {
        if(size && count>SIZE_MAX/size) return 0;auto& self=*static_cast<CurlStream*>(opaque);size_t total=size*count,remaining=total;
        if(!self.hls_ && !validRangeResponse(self.headerStatus_,self.offset_,self.rangeStart_)) return 0;
        while(remaining) {
            pthread_mutex_lock(&self.mutex_);while(self.count_==self.capacity_ && !self.stop_) pthread_cond_wait(&self.changed_,&self.mutex_);
            if(self.stop_) {pthread_mutex_unlock(&self.mutex_);return 0;}
            size_t at=(self.read_+self.count_)%self.capacity_,n=self.capacity_-self.count_;if(n>self.capacity_-at) n=self.capacity_-at;if(n>remaining) n=remaining;
            if(self.faultAt_ && !self.faulted_) n=std::min(n,(size_t)(self.faultAt_-self.downloaded_));
            memcpy(self.data_+at,bytes,n);bytes+=n;remaining-=n;self.count_+=n;self.downloaded_+=n;bool interrupt=self.faultAt_ && !self.faulted_ && self.downloaded_>=self.faultAt_;if(interrupt) self.faulted_=true;pthread_cond_broadcast(&self.changed_);pthread_mutex_unlock(&self.mutex_);
            if(interrupt) {self.log_("STREAM diagnostic_interruption=1 received=%llu",(unsigned long long)self.downloaded_);return 0;}
        }return total;
    }
    static void* entry(void* opaque) {
        auto& self=*static_cast<CurlStream*>(opaque);
        if(self.hls_) {self.hlsWork();return nullptr;}
        CURLcode rc=CURLE_FAILED_INIT;long status=0;unsigned retries=0;
        for(;;) {
        bool previouslyFaulted=self.faulted_;CURL* curl=curl_easy_init();uint64_t before=0;int64_t requestOffset=0;
        pthread_mutex_lock(&self.mutex_);before=self.downloaded_;requestOffset=self.offset_;pthread_mutex_unlock(&self.mutex_);
        if(curl) {
            console_curl_setup(curl);CURLcode network=console_curl_apply_network(curl,nullptr);
            if(network!=CURLE_OK) {curl_easy_cleanup(curl);rc=network;break;}
            curl_easy_setopt(curl,CURLOPT_URL,self.url_.c_str());curl_easy_setopt(curl,CURLOPT_USERAGENT,self.agent_.c_str());
            curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,15L);curl_easy_setopt(curl,CURLOPT_LOW_SPEED_LIMIT,1L);curl_easy_setopt(curl,CURLOPT_LOW_SPEED_TIME,30L);
            curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(curl,CURLOPT_MAXREDIRS,3L);curl_easy_setopt(curl,CURLOPT_PROTOCOLS_STR,"https");curl_easy_setopt(curl,CURLOPT_REDIR_PROTOCOLS_STR,"https");
            curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);curl_easy_setopt(curl,CURLOPT_FAILONERROR,1L);
            curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,+[](void* p,curl_off_t,curl_off_t,curl_off_t,curl_off_t)->int {return static_cast<CurlStream*>(p)->cancelled_.load()?1:0;});curl_easy_setopt(curl,CURLOPT_XFERINFODATA,&self);
            curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION,header);curl_easy_setopt(curl,CURLOPT_HEADERDATA,&self);
            std::string range=std::to_string(requestOffset)+"-";if(requestOffset) curl_easy_setopt(curl,CURLOPT_RANGE,range.c_str());
            curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write);curl_easy_setopt(curl,CURLOPT_WRITEDATA,&self);rc=curl_easy_perform(curl);curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);curl_easy_cleanup(curl);
        }
        pthread_mutex_lock(&self.mutex_);uint64_t received=self.downloaded_-before;int64_t length=self.length_;bool stopped=self.stop_;pthread_mutex_unlock(&self.mutex_);
        int64_t resume=requestOffset+(int64_t)received;
        bool shortResponse=rc==CURLE_OK && length>=0 && resume<length;
        bool transient=(self.faulted_ && !previouslyFaulted) || rc==CURLE_RECV_ERROR || rc==CURLE_SEND_ERROR || rc==CURLE_OPERATION_TIMEDOUT || rc==CURLE_PARTIAL_FILE || rc==CURLE_GOT_NOTHING || rc==CURLE_HTTP2 || rc==CURLE_HTTP2_STREAM || (rc==CURLE_HTTP_RETURNED_ERROR && (status==429 || status>=500));
        if(stopped || self.cancelled_ || (!shortResponse && !transient)) break;
        if(length>=0 && resume==length) {rc=CURLE_OK;break;}
        if(retries>=8) {if(shortResponse) rc=CURLE_PARTIAL_FILE;break;}
        ++retries;self.log_("STREAM reconnect attempt=%u rc=%d status=%ld offset=%lld length=%lld buffered_capacity=%zu",retries,(int)rc,status,(long long)resume,(long long)length,self.capacity_);
        pthread_mutex_lock(&self.mutex_);self.offset_=resume;self.headersReady_=false;pthread_mutex_unlock(&self.mutex_);
        // Keep unread bytes in the ring. Resume AFTER all accepted bytes, never after
        // just the consumed cursor, so buffered audio/video is neither lost nor duplicated.
        for(unsigned n=0;n<std::min(20u,retries*3) && !self.cancelled_;++n) usleep(50000);
        if(self.cancelled_) {rc=CURLE_ABORTED_BY_CALLBACK;break;}
        }

        pthread_mutex_lock(&self.mutex_);self.done_=true;self.failed_=rc!=CURLE_OK && !self.stop_;pthread_cond_broadcast(&self.changed_);pthread_mutex_unlock(&self.mutex_);
        self.log_("STREAM download_end rc=%d status=%ld",(int)rc,status);return nullptr;
    }
    void hlsWork() {
        uint64_t next=0;bool initial=true,failed=false;unsigned segments=0,retries=0;std::string initialization;
        while(!cancelled_) {
            smarttube::Request request;request.url=url_;request.userAgent=agent_;request.httpGet=true;
            auto playlist=performRequest(request,smarttube::LiveHlsMediaLimit,15,&cancelled_);smarttube::LiveHlsMediaInfo media;std::string error;
            if(!playlist.error.empty() || !smarttube::validateLiveHlsMedia(playlist.body,url_,error,&media) || media.byteRanges) {
                if(cancelled_) break;if(++retries<4) {for(int n=0;n<10 && !cancelled_;++n) usleep(100000);continue;}
                log_("HLS playlist_failed HTTP=%ld valid=%d byteranges=%d",playlist.status,error.empty(),media.byteRanges);failed=true;break;
            }retries=0;
            if(initial) {next=media.sequence+(media.segmentUrls.size()>3?media.segmentUrls.size()-3:0);initial=false;log_("HLS begin fragmented=%d window_segments=%zu target_seconds=%u",media.fragmented,media.segmentUrls.size(),media.targetSeconds);}
            if(next<media.sequence) {log_("HLS window_gap missed=%llu",(unsigned long long)(media.sequence-next));failed=true;break;}
            if(!media.initialization.empty() && media.initialization!=initialization) {request.url=media.initialization;auto bytes=performRequest(request,4*1024*1024,15,&cancelled_);if(!bytes.error.empty() || write(bytes.body.data(),1,bytes.body.size(),this)!=bytes.body.size()) {failed=!cancelled_;break;}initialization=media.initialization;}
            for(size_t i=0;i<media.segmentUrls.size() && !cancelled_;++i) {
                uint64_t sequence=media.sequence+i;if(sequence<next) continue;request.url=media.segmentUrls[i];
                auto bytes=performRequest(request,16*1024*1024,15,&cancelled_);
                if(!bytes.error.empty() || bytes.body.empty()) {log_("HLS segment_failed HTTP=%ld sequence=%llu",bytes.status,(unsigned long long)sequence);failed=!cancelled_;break;}
                if(write(bytes.body.data(),1,bytes.body.size(),this)!=bytes.body.size()) break;
                next=sequence+1;++segments;if(segments%10==0) log_("HLS progress segments=%u",segments);
            }
            if(failed || media.ended) break;
            unsigned wait=std::min(20u,std::max(2u,media.targetSeconds*5));for(unsigned n=0;n<wait && !cancelled_;++n) usleep(100000);
        }
        pthread_mutex_lock(&mutex_);done_=true;failed_=failed;pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);log_("HLS ended segments=%u failed=%d cancelled=%d",segments,failed,cancelled_.load());
    }

public:
    explicit CurlStream(BenchLog log,uint64_t diagnosticFaultAt=0):faultAt_(diagnosticFaultAt),log_(log) {}
    void configure(unsigned bitrate,bool audio,bool live,std::function<void(bool)> buffering={},unsigned level=1) {
        auto p=BufferPolicy::media(bitrate,audio,live,level);capacity_=p.capacity;startupBytes_=p.startup;resumeBytes_=p.resume;buffering_=std::move(buffering);
        log_("BUFFER policy audio=%d live=%d bitrate=%u capacity=%zu startup=%zu resume=%zu",audio,live,bitrate,capacity_,startupBytes_,resumeBytes_);
    }
    bool ready() {pthread_mutex_lock(&mutex_);bool ready=count_>=startupBytes_ || done_ || stop_;pthread_mutex_unlock(&mutex_);return ready;}
    bool start(const std::string& url,const std::string& agent,bool hls=false) {
        hls_=hls;
        if(url.compare(0,8,"https://")) return false;data_=static_cast<uint8_t*>(malloc(capacity_));if(!data_) return false;url_=url;agent_=agent;
        pthread_attr_t attr;int rc=pthread_attr_init(&attr);if(rc) return false;rc=pthread_attr_setstacksize(&attr,2*1024*1024);if(!rc) rc=pthread_create(&thread_,&attr,entry,this);pthread_attr_destroy(&attr);started_=rc==0;return started_;
    }
    static int read(void* opaque,uint8_t* bytes,int size) {
        auto& self=*static_cast<CurlStream*>(opaque);pthread_mutex_lock(&self.mutex_);
        bool stalled=self.primed_ && !self.count_ && !self.done_ && !self.stop_;
        size_t target=!self.primed_?self.startupBytes_:stalled?self.resumeBytes_:1;
        double began=stalled?benchmarkTime():0;
        if(stalled) {if(self.buffering_) self.buffering_(true);self.log_("BUFFER wait_begin resume_bytes=%zu",target);}
        while(self.count_<target && !self.done_ && !self.stop_) pthread_cond_wait(&self.changed_,&self.mutex_);
        if(stalled) {if(self.buffering_) self.buffering_(false);self.log_("BUFFER wait_end milliseconds=%.1f bytes=%zu",(benchmarkTime()-began)*1000,self.count_);}
        self.primed_=true;
        size_t n=self.count_;if(n>(size_t)size) n=(size_t)size;if(n>self.capacity_-self.read_) n=self.capacity_-self.read_;
        if(n) {memcpy(bytes,self.data_+self.read_,n);self.read_=(self.read_+n)%self.capacity_;self.count_-=n;self.consumed_+=n;}
        bool failed=self.failed_;pthread_cond_broadcast(&self.changed_);pthread_mutex_unlock(&self.mutex_);return n?(int)n:failed?AVERROR(EIO):AVERROR_EOF;
    }
    static int64_t seek(void* opaque,int64_t offset,int origin) {
        auto& self=*static_cast<CurlStream*>(opaque);if(self.hls_) return AVERROR(ENOSYS);
        pthread_mutex_lock(&self.mutex_);
        if(origin==AVSEEK_SIZE) {while(!self.headersReady_ && !self.done_ && !self.stop_) pthread_cond_wait(&self.changed_,&self.mutex_);int64_t length=self.length_;pthread_mutex_unlock(&self.mutex_);return length>=0?length:AVERROR(ENOSYS);}
        origin &= ~AVSEEK_FORCE;int64_t base=origin==SEEK_SET?0:origin==SEEK_CUR?(int64_t)self.consumed_:origin==SEEK_END?self.length_:-1;
        if(base<0 || (offset>0 && base>INT64_MAX-offset) || (offset<0 && offset< -base)) {pthread_mutex_unlock(&self.mutex_);return AVERROR(EINVAL);}
        int64_t target=base+offset;if(self.length_>=0 && target>self.length_) {pthread_mutex_unlock(&self.mutex_);return AVERROR(EINVAL);}
        if(target>=(int64_t)self.consumed_ && (uint64_t)(target-self.consumed_)<=self.count_) {size_t skip=(size_t)(target-self.consumed_);self.read_=(self.read_+skip)%self.capacity_;self.count_-=skip;self.consumed_=target;pthread_cond_broadcast(&self.changed_);pthread_mutex_unlock(&self.mutex_);return target;}
        pthread_mutex_unlock(&self.mutex_);self.cancel();if(self.started_) {pthread_join(self.thread_,nullptr);self.started_=false;}
        pthread_mutex_lock(&self.mutex_);self.primed_=false;self.read_=self.count_=0;self.offset_=target;self.consumed_=target;self.stop_=self.done_=self.failed_=self.headersReady_=false;self.cancelled_=false;pthread_mutex_unlock(&self.mutex_);
        if(target==self.length_) {self.done_=true;return target;}
        pthread_attr_t attr;int rc=pthread_attr_init(&attr);if(rc) {pthread_mutex_lock(&self.mutex_);self.done_=self.failed_=true;pthread_cond_broadcast(&self.changed_);pthread_mutex_unlock(&self.mutex_);return AVERROR(EIO);}rc=pthread_attr_setstacksize(&attr,2*1024*1024);if(!rc) rc=pthread_create(&self.thread_,&attr,entry,&self);pthread_attr_destroy(&attr);self.started_=rc==0;
        if(rc) {pthread_mutex_lock(&self.mutex_);self.done_=self.failed_=true;pthread_cond_broadcast(&self.changed_);pthread_mutex_unlock(&self.mutex_);return AVERROR(EIO);}
        self.log_("STREAM range_seek offset=%lld",(long long)target);return target;
    }
    void cancel() {cancelled_=true;pthread_mutex_lock(&mutex_);stop_=true;pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);}
    ~CurlStream() {close();pthread_cond_destroy(&changed_);pthread_mutex_destroy(&mutex_);}
    void close() {
        if(!data_ && !started_) return;cancel();
        if(started_) {pthread_join(thread_,nullptr);started_=false;}
        log_("STREAM closed downloaded=%llu consumed=%llu bounded_capacity=%zu",(unsigned long long)downloaded_,(unsigned long long)consumed_,capacity_);free(data_);data_=nullptr;
    }
};
}
