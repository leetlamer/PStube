#include "content_services.h"
#include "seek_timeline.h"
#include <cassert>
#include <cstdio>
using namespace probe;
static smarttube::Json parse(const char* text) {smarttube::Json out;std::string error;assert(smarttube::parseJson(text,out,error));return out;}
int main() {
    auto ranges=sponsorRanges(parse(R"([{"category":"sponsor","actionType":"skip","segment":[1,2]},{"category":"intro","actionType":"skip","segment":[2,3]},{"category":"sponsor","actionType":"skip","segment":[3,2]}])"));assert(ranges.size()==1);
    int16_t block[2048];std::fill(block,block+2048,42);muteSponsorSamples(block,1024,1-512.0/48000,ranges);assert(block[1023]==42 && block[1024]==0 && block[2047]==0);
    std::fill(block,block+2048,42);muteSponsorSamples(block,1024,2-512.0/48000,ranges);assert(block[1023]==0 && block[1024]==42 && block[2047]==42);
    auto branding=dearrowBranding(parse(R"({"titles":[{"title":"Untrusted","original":false,"votes":-2},{"title":"A >clear title","original":false,"votes":1}],"thumbnails":[{"timestamp":42.5,"original":false,"votes":0}]})"));assert(branding.title=="A clear title" && branding.timestamp==42.5);
    assert(dearrowBranding(parse(R"({"titles":[{"title":"Original","original":true,"votes":10}]})")).title.empty());
    assert(validRangeResponse(200,0,-1) && validRangeResponse(206,100,100));assert(!validRangeResponse(200,100,-1) && !validRangeResponse(206,100,0) && !validRangeResponse(302,0,-1));
    auto skips=sponsorSkips(parse(R"([{"category":"sponsor","actionType":"skip","segment":[5,10],"videoDuration":60},{"category":"intro","actionType":"skip","segment":[0,7],"videoDuration":60},{"category":"sponsor","actionType":"mute","segment":[12,15]},{"category":"outro","actionType":"skip","segment":[20,30],"videoDuration":80}])"),31,1,60);assert(skips.size()==1 && skips[0].start==0 && skips[0].end==10);
    assert(sponsorSkips(parse(R"([{"category":"sponsor","actionType":"skip","segment":[1,2]}])"),1,3,60).empty());
    auto markers=sponsorSkips(parse(R"([{"category":"intro","actionType":"skip","segment":[0,5]},{"category":"selfpromo","actionType":"skip","segment":[10,15]}])"),31,0,60,false);assert(markers.size()==2 && markers[0].category==2 && markers[1].category==1);
    assert(sponsorQuery(1)=="%5B%22sponsor%22%5D");assert(playbackTime(3661)=="1:01:01" && playbackTime(65)=="1:05");assert(clampSeek(-3,60)==0 && clampSeek(100,60)<60);
    puts("PASS: SponsorBlock bounded parsing/stereo sample boundaries; DeArrow trusted alternatives/original fallback");
}
