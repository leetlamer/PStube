#pragma once
namespace probe {
class DocumentJob {
    pthread_t thread_{};std::atomic<bool> done_{false},cancelled_{false};bool running_=false;
    smarttube::Request request_;Document response_;
    static void* entry(void* p) {auto& self=*static_cast<DocumentJob*>(p);self.response_=performRequest(self.request_,2*1024*1024,12,&self.cancelled_);self.done_=true;return nullptr;}
public:
    bool busy() const {return running_;}
    bool start(const smarttube::Request& request) {if(running_) return false;request_=request;done_=false;cancelled_=false;pthread_attr_t attr;if(pthread_attr_init(&attr)) return false;int rc=pthread_attr_setstacksize(&attr,2*1024*1024);if(!rc) rc=pthread_create(&thread_,&attr,entry,this);pthread_attr_destroy(&attr);running_=rc==0;return running_;}
    bool take(Document& response) {if(!running_ || !done_) return false;pthread_join(thread_,nullptr);running_=false;response=std::move(response_);return true;}
    void cancel() {cancelled_=true;if(running_) {pthread_join(thread_,nullptr);running_=false;}}
    ~DocumentJob() {cancel();}
};
}
