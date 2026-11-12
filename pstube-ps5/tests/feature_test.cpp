#include "stream_selection.h"
#include "shared/app_state.h"
#include <assert.h>
#include <stdio.h>
static smarttube::Stream track(int height,int fps,const char* codec,int itag) {
    smarttube::Stream result;result.height=height;result.width=height*16/9;result.fps=fps;result.mime=codec;result.itag=itag;result.url="https://example.invalid/media";result.adaptive=true;result.bitrate=128000;return result;
}
int main() {
    smarttube::PlayerInfo info;info.streams={track(1080,30,"video/webm; codecs=vp9",248),track(1440,60,"video/webm; codecs=vp9",308),track(2160,60,"video/webm; codecs=vp9",315),track(0,0,"audio/webm; codecs=opus",251)};
    assert(probe::adaptiveChoice(info,1080,60).video==0);assert(probe::adaptiveChoice(info,1440,60).video==1);assert(probe::adaptiveChoice(info,2160,60).video==2);
    assert(probe::adaptiveChoice(info,2160,30).video==0);
    info.streams[2].mime="video/webm; codecs=vp09.02";assert(probe::adaptiveChoice(info,2160,60).video==1);
    info.streams[1].requiresSignature=true;assert(probe::adaptiveChoice(info,2160,60).video==0);
    info.streams[3].autoDubbed=true;assert(probe::adaptiveChoice(info,2160,60).audio==-1);
    info.streams.push_back(track(0,0,"audio/mp4; codecs=mp4a",140));assert(probe::adaptiveChoice(info,2160,60).audio==4);
    smarttube::Library saved;assert(saved.preferences.height==1080);saved.preferences.decoderThreads=10;saved.preferences.height=2160;saved.preferences.native4k=true;saved.preferences.debugMetrics=true;saved.preferences.endMode=smarttube::EndMode::Loop;saved.preferences.sponsorMute=true;saved.preferences.sponsorCategories=17;saved.preferences.sponsorMinimum=3;saved.preferences.sponsorNotices=false;saved.preferences.dearrow=true;saved.preferences.dearrowTitles=false;saved.preferences.dearrowThumbnails=true;
    saved.watched({"video","A title","A channel","https://example.invalid/image"});saved.toggleFavorite(saved.history[0]);assert(saved.save("build/feature-library.json"));
    smarttube::Library loaded;assert(loaded.load("build/feature-library.json"));assert(loaded.preferences.decoderThreads==10);assert(loaded.preferences.height==2160 && loaded.preferences.native4k && loaded.preferences.debugMetrics && loaded.preferences.endMode==smarttube::EndMode::Loop);assert(loaded.preferences.sponsorMute && loaded.preferences.dearrow && !loaded.preferences.dearrowTitles && loaded.preferences.dearrowThumbnails && loaded.preferences.sponsorCategories==17 && loaded.preferences.sponsorMinimum==3 && !loaded.preferences.sponsorNotices);assert(loaded.history.size()==1 && loaded.favorites.size()==1);remove("build/feature-library.json");
    puts("PASS: resolution/fps/codec/language policy and saved 4K settings/favorites/history");
}
