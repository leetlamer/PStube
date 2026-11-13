#include "shared/app_ui.h"
#include <pthread.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <cassert>
#include <cstdio>
using CURL=int;
static unsigned console_curl_network_generation() {return 1;}
static CURL* curl_easy_init() {return new int(1);}static void curl_easy_cleanup(CURL* p) {delete p;}
namespace smarttube {const char* tvUserAgent(unsigned) {return "Fixture TV";}std::shared_ptr<Image> decodeThumbnail(const std::string&,int w,int h) {auto out=std::make_shared<Image>();out->width=w;out->height=h;return out;}}
namespace probe {
struct Document {std::string body,error;};
static double benchmarkTime() {return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
static Document performRequest(const smarttube::Request& request,size_t,long,const std::atomic<bool>* cancelled,CURL*) {bool branding=request.url.find("/api/branding")!=std::string::npos;for(int i=0;i<(branding?200:30) && !*cancelled;++i) std::this_thread::sleep_for(std::chrono::milliseconds(2));return *cancelled?Document{"","Cancelled"}:Document{request.url.find("youtubei/v1/next")!=std::string::npos?R"({"videoMetadataRenderer":{"likeButton":{"likeButtonRenderer":{"likeCount":42}},"owner":{"videoOwnerRenderer":{"title":{"simpleText":"Real creator"},"subscriberCountText":{"simpleText":"12K subscribers"}}}}})":branding?R"({"titles":[{"title":"Alternative title","original":false,"votes":1}]})":"fixture",""};}
}
#include "thumbnail_loader.h"
int main() {
    probe::ThumbnailLoader loader;std::vector<smarttube::Video> items;for(int i=0;i<20;++i) items.push_back({"old"+std::to_string(i),"","","https://fixture.invalid/thumbnail"});
    std::map<std::string,std::shared_ptr<smarttube::Image>> cache;double start=probe::benchmarkTime();for(int i=0;i<1000;++i) loader.visible(items,i%10,8,cache);assert(probe::benchmarkTime()-start<0.2); // Queueing never waits for the slow transport.
    loader.clear();std::vector<smarttube::Video> next={{"new","","","https://fixture.invalid/new"}};loader.visible(next,0,1,cache);
    for(int n=0;n<200 && !cache.count("new");++n) {std::this_thread::sleep_for(std::chrono::milliseconds(3));loader.drain(cache,{"new"});}
    assert(cache.size()==1 && cache.count("new"));loader.clear();cache.clear();loader.dearrow(true);next.clear();for(int i=0;i<3;++i) next.push_back({"brand"+std::to_string(i),"Original","","https://fixture.invalid/new"});start=probe::benchmarkTime();loader.visible(next,0,next.size(),cache);
    while(cache.size()<3 && probe::benchmarkTime()-start<0.35) {std::this_thread::sleep_for(std::chrono::milliseconds(3));loader.drain(cache,{"brand0","brand1","brand2"},&next);}assert(cache.size()==3); // Slow optional branding must not block original images.
    for(int i=0;i<600 && loader.brandingUpdates()<3;++i) {std::this_thread::sleep_for(std::chrono::milliseconds(3));loader.drain(cache,{"brand0","brand1","brand2"},&next);}assert(loader.brandingUpdates()==3 && next[0].title=="Alternative title");loader.clear();next={{"metatest001","One","Channel","https://fixture.invalid/new"},{"metatest002","Two","Channel","https://fixture.invalid/new"}};loader.visible(next,0,2,cache);for(int i=0;i<400 && next[1].channel!="Real creator";++i) {std::this_thread::sleep_for(std::chrono::milliseconds(3));loader.drain(cache,{},&next);}assert(next[0].channel=="Real creator" && next[1].channel=="Real creator" && next[1].likes=="42" && next[1].subscribers=="12K subscribers");loader.shutdown();loader.shutdown();puts("PASS: image queue remains nonblocking during navigation; stale generation results discarded; shutdown joins workers");
}
