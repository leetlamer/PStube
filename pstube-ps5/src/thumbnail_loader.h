#pragma once
#include <deque>
#include <set>
#include "content_services.h"
namespace probe {
// Two image workers plus a separate optional DeArrow worker keep navigation and originals responsive.
class ThumbnailLoader {
    struct Job {std::string id,url;unsigned generation;bool needTitle=false,dearrow=false,titles=true,thumbnails=true;};
    struct Result {std::string id;std::shared_ptr<smarttube::Image> image;unsigned generation;std::string title{},author{};bool alternative=false;};
    pthread_mutex_t mutex_=PTHREAD_MUTEX_INITIALIZER;pthread_cond_t changed_=PTHREAD_COND_INITIALIZER;
    struct Worker {ThumbnailLoader* owner=nullptr;bool branding=false;};Worker context_[3]{};pthread_t workers_[3]{};unsigned started_=0,generation_=0;bool stop_=false,dearrow_=false,titles_=true,thumbnails_=true;
    std::deque<Job> jobs_,branding_;std::set<std::string> branded_;std::map<std::string,std::string> alternatives_;std::atomic<unsigned> brandingRequests_{0},brandingSuccess_{0},brandingUpdates_{0},brandingCompleted_{0},brandingTitles_{0},brandingImages_{0};std::deque<Result> results_;std::set<std::string> pending_;std::map<std::string,double> retry_;
    std::atomic<bool> cancelled_{false};
    static void* entry(void* p) {auto& context=*static_cast<Worker*>(p);context.owner->work(context.branding);return nullptr;}
    void work(bool brandingWorker) {
        CURL* connection=curl_easy_init();unsigned networkEpoch=console_curl_network_generation();
        for(;;) {
            pthread_mutex_lock(&mutex_);auto& queue=brandingWorker?branding_:jobs_;while(queue.empty() && !stop_) pthread_cond_wait(&changed_,&mutex_);
            if(stop_) {pthread_mutex_unlock(&mutex_);break;}Job job=queue.front();queue.pop_front();if(job.generation!=generation_) {pthread_mutex_unlock(&mutex_);continue;}pthread_mutex_unlock(&mutex_);
            unsigned currentNetwork=console_curl_network_generation();if(networkEpoch!=currentNetwork) {if(connection) curl_easy_cleanup(connection);connection=curl_easy_init();networkEpoch=currentNetwork;}
            smarttube::Request request;request.url=job.url;request.httpGet=true;request.userAgent="PStube PS5";
            if(!brandingWorker) {
            auto document=performRequest(request,512*1024,8,&cancelled_,connection);
            auto image=document.error.empty()?smarttube::decodeThumbnail(document.body,384,216):nullptr;
            pthread_mutex_lock(&mutex_);if(job.generation==generation_) {pending_.erase(job.id);if(!image) retry_[job.id]=benchmarkTime()+30;if(results_.size()<32) results_.push_back({job.id,image,job.generation});}pthread_mutex_unlock(&mutex_);
            if(job.needTitle && !cancelled_) {
                request.url="https://www.youtube.com/oembed?url=https%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3D"+job.id+"&format=json";
                auto metadata=performRequest(request,64*1024,6,&cancelled_,connection);smarttube::Json json;std::string error;
                if(smarttube::parseJson(metadata.body,json,error)) {auto title=json.string("title"),author=json.string("author_name");if(!title.empty() && title.size()<1024 && author.size()<256) {pthread_mutex_lock(&mutex_);if(job.generation==generation_ && results_.size()<32) results_.push_back({job.id,{},job.generation,title,author});pthread_mutex_unlock(&mutex_);}}
            }
            if(job.dearrow && !cancelled_) {pthread_mutex_lock(&mutex_);if(started_==3 && job.generation==generation_ && branding_.size()<24 && !branded_.count(job.id)) {branded_.insert(job.id);branding_.push_back(job);pthread_cond_broadcast(&changed_);}pthread_mutex_unlock(&mutex_);}
            }else if(job.dearrow && !cancelled_) {++brandingRequests_;
                request.url="https://sponsor.ajay.app/api/branding?videoID="+job.id;
                auto metadata=performRequest(request,64*1024,6,&cancelled_,connection);smarttube::Json json;std::string error;
                if(metadata.error.empty() && smarttube::parseJson(metadata.body,json,error)) {++brandingSuccess_;
                    auto branding=dearrowBranding(json);std::shared_ptr<smarttube::Image> replacement;
                    if(job.thumbnails && branding.timestamp>=0) {request.url="https://dearrow-thumb.ajay.app/api/v1/getThumbnail?videoID="+job.id+"&time="+std::to_string(branding.timestamp);auto thumbnail=performRequest(request,512*1024,6,&cancelled_,connection);if(thumbnail.error.empty()) replacement=smarttube::decodeThumbnail(thumbnail.body,384,216);}
                    if(!job.titles) branding.title.clear();
                    if(!branding.title.empty() || replacement) {pthread_mutex_lock(&mutex_);if(job.generation==generation_ && results_.size()<32) {if(replacement) alternatives_[job.id]=request.url;results_.push_back({job.id,replacement,job.generation,branding.title,{},true});}pthread_mutex_unlock(&mutex_);}
                }
                ++brandingCompleted_;
            }
        }if(connection) curl_easy_cleanup(connection);
    }
public:
    ThumbnailLoader() {
        for(unsigned i=0;i<3;++i) {context_[started_]={this,i==2};pthread_attr_t attr;if(pthread_attr_init(&attr)) break;int rc=pthread_attr_setstacksize(&attr,2*1024*1024);if(!rc) rc=pthread_create(&workers_[started_],&attr,entry,&context_[started_]);pthread_attr_destroy(&attr);if(!rc) ++started_;else break;}
    }
    void dearrow(bool enabled,bool titles=true,bool thumbnails=true) {pthread_mutex_lock(&mutex_);dearrow_=enabled;titles_=titles;thumbnails_=thumbnails;pthread_mutex_unlock(&mutex_);}
    unsigned brandingTitles() const {return brandingTitles_.load();}
    unsigned brandingImages() const {return brandingImages_.load();}
    unsigned brandingCompleted() const {return brandingCompleted_.load();}
    unsigned brandingRequests() const {return brandingRequests_.load();}
    unsigned brandingSuccess() const {return brandingSuccess_.load();}
    unsigned brandingUpdates() const {return brandingUpdates_.load();}
    unsigned workers() const {return started_;}
    void shutdown() {if(stop_) return;cancelled_=true;pthread_mutex_lock(&mutex_);stop_=true;pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);for(unsigned i=0;i<started_;++i) pthread_join(workers_[i],nullptr);}
    ~ThumbnailLoader() {shutdown();pthread_cond_destroy(&changed_);pthread_mutex_destroy(&mutex_);}
    void clear() {pthread_mutex_lock(&mutex_);++generation_;jobs_.clear();branding_.clear();branded_.clear();alternatives_.clear();results_.clear();pending_.clear();retry_.clear();pthread_mutex_unlock(&mutex_);}
    void visible(const std::vector<smarttube::Video>& items,size_t first,size_t count,const std::map<std::string,std::shared_ptr<smarttube::Image>>& cache) {
        pthread_mutex_lock(&mutex_);std::set<std::string> wanted;
        for(size_t i=first;i<items.size() && i<first+count;++i) wanted.insert(items[i].id);
        for(auto job=jobs_.begin();job!=jobs_.end();) {if(!wanted.count(job->id)) {pending_.erase(job->id);job=jobs_.erase(job);}else ++job;}
        for(size_t i=first;i<items.size() && i<first+count;++i) if(!cache.count(items[i].id) && !pending_.count(items[i].id) && (!retry_.count(items[i].id) || retry_[items[i].id]<=benchmarkTime()) && items[i].thumbnail.compare(0,8,"https://")==0 && jobs_.size()<24) {jobs_.push_back({items[i].id,dearrow_ && thumbnails_ && alternatives_.count(items[i].id)?alternatives_[items[i].id]:items[i].thumbnail,generation_,items[i].title=="Shorts video",dearrow_,titles_,thumbnails_});pending_.insert(items[i].id);}
        pthread_cond_broadcast(&changed_);pthread_mutex_unlock(&mutex_);
    }
    unsigned drain(std::map<std::string,std::shared_ptr<smarttube::Image>>& cache,const std::set<std::string>& visible,std::vector<smarttube::Video>* items=nullptr) {
        unsigned completed=0;pthread_mutex_lock(&mutex_);
        while(!results_.empty()) {auto result=std::move(results_.front());results_.pop_front();if(result.generation!=generation_) continue;if(result.alternative) {++brandingUpdates_;if(result.image) ++brandingImages_;if(!result.title.empty()) ++brandingTitles_;}if(items && !result.title.empty()) for(auto& item:*items) if(item.id==result.id) {item.title=result.title;if(!result.author.empty()) item.channel=result.author;}if(!result.image) continue;
            if(cache.size()>=64 && !cache.count(result.id)) {auto old=cache.begin();while(old!=cache.end() && visible.count(old->first)) ++old;if(old!=cache.end()) cache.erase(old);else continue;}
            cache[result.id]=std::move(result.image);++completed;
        }pthread_mutex_unlock(&mutex_);return completed;
    }
};
}
