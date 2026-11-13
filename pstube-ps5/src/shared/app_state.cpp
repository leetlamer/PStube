#include "app_state.h"
#include <algorithm>
#include <stdio.h>
namespace smarttube {
namespace {
bool valid(const Video& v) {return !v.id.empty() && v.id.size()<=64 && v.title.size()<=1024 && v.channel.size()<=512 && v.thumbnail.size()<=2048 && v.badge.size()<=32 && v.views.size()<=512 && v.published.size()<=256 && v.likes.size()<=128 && v.dislikes.size()<=128 && v.subscribers.size()<=256 && v.resumeMs<=43200000;}
std::string videoJson(const Video& v) {return "{\"id\":"+quoteJson(v.id)+",\"title\":"+quoteJson(v.title)+",\"channel\":"+quoteJson(v.channel)+",\"thumbnail\":"+quoteJson(v.thumbnail)+",\"badge\":"+quoteJson(v.badge)+",\"views\":"+quoteJson(v.views)+",\"published\":"+quoteJson(v.published)+",\"likes\":"+quoteJson(v.likes)+",\"dislikes\":"+quoteJson(v.dislikes)+",\"subscribers\":"+quoteJson(v.subscribers)+",\"resumeMs\":"+std::to_string(v.resumeMs)+"}";}
void readList(const Json* node,std::vector<Video>& out) {
    if(!node || node->type!=Json::Array) return;
    for(const auto& item:node->children) {
        Video v{item.string("id"),item.string("title"),item.string("channel"),item.string("thumbnail")};
        v.views=item.string("views");v.published=item.string("published");v.likes=item.string("likes");v.dislikes=item.string("dislikes");v.subscribers=item.string("subscribers");v.badge=item.string("badge");const auto position=item.integer("resumeMs");if(position>0 && position<=43200000) v.resumeMs=(unsigned)position;
        if(valid(v) && out.size()<40) out.push_back(v);
    }
}
void prepend(std::vector<Video>& out,const Video& video) {
    if(!valid(video)) return;
    out.erase(std::remove_if(out.begin(),out.end(),[&](const Video& v){return v.id==video.id;}),out.end());
    out.insert(out.begin(),video);if(out.size()>40) out.resize(40);
}
}
void PlaybackQuality::begin(const Preferences& preferences,int overrideHeight) {
    height=qualityIndex(overrideHeight)>=0?overrideHeight:defaultQuality(preferences.height);poorWindows=0;
}
int PlaybackQuality::observe(int sourceHeight,unsigned sourceFps,unsigned delivered,double seconds,bool eligible,bool late,bool starved) {
    // Two sustained poor windows, after startup, with actual lateness. Do not
    // demote paused/loading playback or let a UHD recovery become a 480p default.
    if(!eligible || sourceHeight<=1080 || !sourceFps || seconds<4 || seconds>8 || !late || delivered/seconds>=sourceFps*0.70) {poorWindows=0;return 0;}
    if(!starved && ++poorWindows<2) return 0;
    poorWindows=0;height=sourceHeight>1440?1440:1080;return height;
}
bool Library::load(const char* path) {
    FILE* f=fopen(path,"rb");if(!f) return false;
    std::string bytes;char buffer[4096];size_t n;
    while((n=fread(buffer,1,sizeof(buffer),f))!=0 && bytes.size()+n<=128*1024) bytes.append(buffer,n);
    const bool ok=feof(f) && !ferror(f);fclose(f);
    Json document;std::string error;if(!ok || !parseJson(bytes,document,error) || document.type!=Json::Object) return false;
    Library next;readList(document.get("favorites"),next.favorites);readList(document.get("history"),next.history);
    // Ignore legacy saved searches so each launch starts with an empty Home.
    const int height=document.integer("height",SupportedVideoHeight);
    next.preferences.sponsorMute=document.integer("ps5SponsorSkip",document.integer("ps5SponsorMute",0))==1;next.preferences.sponsorCategories=(unsigned)std::max(0,std::min(31,document.integer("ps5SponsorCategories",1)));next.preferences.sponsorMinimum=(unsigned)std::max(0,std::min(5,document.integer("ps5SponsorMinimum",0)));next.preferences.sponsorNotices=document.integer("ps5SponsorNotices",1)==1;next.preferences.dearrow=document.integer("ps5DeArrow",0)==1;next.preferences.dearrowTitles=document.integer("ps5DeArrowTitles",1)==1;next.preferences.dearrowThumbnails=document.integer("ps5DeArrowThumbnails",1)==1;
    next.preferences.height=defaultQuality(height);next.preferences.native4k=document.integer("ps5Native4k",0)==1;next.preferences.debugMetrics=document.integer("ps5Debug",0)==1;
    next.preferences.audioLanguage=document.string("audioLanguage")=="en"?"en":"original";
    const bool currentDefaults=document.integer("playbackDefaults",0)>=52;
    next.preferences.softwareCodecs=currentDefaults?document.integer("softwareCodecs",1)==1:true;
    next.preferences.preferAv1=currentDefaults && document.integer("preferAv1",0)==1;
    next.preferences.scalerThreads=currentDefaults && document.integer("scalerThreads",2)==1?1:2;
    unsigned workers=(unsigned)document.integer("ps5DecoderThreads",8);next.preferences.decoderThreads=workers==6 || workers==10?workers:8;
    next.preferences.bufferLevel=(unsigned)std::max(0,std::min(3,document.integer("ps5BufferLevel",1)));
    next.preferences.maxFps=document.integer("fpsDefaults",0)>=53 && document.integer("maxFps",60)==30?30:60;
    next.preferences.fastStart=false; // Migrate v0.17: native demuxer rejected the fragmented layout.
    const int end=document.integer("endMode",0);next.preferences.endMode=end>=0 && end<=2?static_cast<EndMode>(end):EndMode::AutoNext;
    const auto seconds=document.integer("playbackMode",0)>=14?document.integer("seconds",0):0;
    if(seconds==0 || seconds==30 || seconds==60 || seconds==120 || seconds==300) next.preferences.seconds=(unsigned)seconds;
    *this=std::move(next);return true;
}
bool Library::save(const char* path) const {
    std::string bytes="{\"playbackDefaults\":52,\"fpsDefaults\":53,\"scalerThreads\":"+std::to_string(preferences.scalerThreads==1?1:2)+",\"endMode\":"+std::to_string(static_cast<int>(preferences.endMode))+",\"preferAv1\":"+std::to_string(preferences.preferAv1?1:0)+",\"audioLanguage\":"+quoteJson(preferences.audioLanguage)+",\"softwareCodecs\":"+std::to_string(preferences.softwareCodecs?1:0)+",\"maxFps\":"+std::to_string(preferences.maxFps)+",\"fastStart\":"+std::to_string(preferences.fastStart?1:0)+",\"playbackMode\":14,\"height\":"+std::to_string(defaultQuality(preferences.height))+",\"seconds\":"+std::to_string(preferences.seconds)+",\"autoplay\":"+(preferences.endMode==EndMode::AutoNext?"1":"0");
    bytes+=",\"ps5SponsorSkip\":"+std::to_string(preferences.sponsorMute?1:0)+",\"ps5SponsorCategories\":"+std::to_string(preferences.sponsorCategories)+",\"ps5SponsorMinimum\":"+std::to_string(preferences.sponsorMinimum)+",\"ps5SponsorNotices\":"+std::to_string(preferences.sponsorNotices?1:0)+",\"ps5DeArrow\":"+std::to_string(preferences.dearrow?1:0)+",\"ps5DeArrowTitles\":"+std::to_string(preferences.dearrowTitles?1:0)+",\"ps5DeArrowThumbnails\":"+std::to_string(preferences.dearrowThumbnails?1:0);
    bytes+=",\"ps5DecoderThreads\":"+std::to_string(preferences.decoderThreads)+",\"ps5Native4k\":"+std::to_string(preferences.native4k?1:0)+",\"ps5Debug\":"+std::to_string(preferences.debugMetrics?1:0);
    bytes+=",\"ps5BufferLevel\":"+std::to_string(preferences.bufferLevel);
    for(const auto& item:{std::make_pair("favorites",&favorites),std::make_pair("history",&history)}) {
        bytes+=",\""+std::string(item.first)+"\":[";bool first=true;
        for(const auto& video:*item.second) {if(!first) bytes+=",";first=false;bytes+=videoJson(video);}bytes+="]";
    }
    bytes+="}";if(bytes.size()>128*1024) return false;const std::string temporary=std::string(path)+".part";
    FILE* f=fopen(temporary.c_str(),"wb");if(!f) return false;
    bool ok=fwrite(bytes.data(),1,bytes.size(),f)==bytes.size();if(fclose(f)!=0) ok=false;
    if(!ok || rename(temporary.c_str(),path)!=0) {remove(temporary.c_str());return false;}return true;
}
bool Library::favorite(const std::string& id) const {for(const auto& v:favorites) if(v.id==id) return true;return false;}
void Library::toggleFavorite(const Video& video) {
    if(favorite(video.id)) favorites.erase(std::remove_if(favorites.begin(),favorites.end(),[&](const Video& v){return v.id==video.id;}),favorites.end());
    else prepend(favorites,video);
}
void Library::watched(const Video& video) {auto item=video;item.resumeMs=resume(video.id);prepend(history,item);}
void Library::remember(const std::string& id,unsigned milliseconds) {for(auto& item:history) if(item.id==id) item.resumeMs=std::min(43200000u,milliseconds);}
unsigned Library::resume(const std::string& id) const {for(const auto& item:history) if(item.id==id) return item.resumeMs;return 0;}
int qualityHeight(int index) {static const int values[]={144,240,360,480,720,1080,1440,2160};return index>=0 && index<QualityCount?values[index]:240;}
int qualityIndex(int height) {for(int i=0;i<QualityCount;++i) if(qualityHeight(i)==height) return i;return -1;}
int defaultQuality(int height) {return qualityIndex(height)>=0?std::min(height,SupportedVideoHeight):SupportedVideoHeight;}
int cycleDefaultQuality(int height,int direction) {
    const int count=qualityIndex(SupportedVideoHeight)+1;
    const int index=qualityIndex(defaultQuality(height));
    return qualityHeight((index+(direction<0?count-1:1))%count);
}
int cycleQuality(int height,int direction) {int index=qualityIndex(height);if(index<0) index=1;return qualityHeight((index+(direction<0?QualityCount-1:1))%QualityCount);}
std::string qualityLabel(int height) {return std::to_string(height)+"p"+(height<=1080?"":height==1440?" (2K)":" (4K)");}
size_t videoBudget(int height) {return (height<=240?8u:height<=480?12u:16u)*1024*1024;}

void Keyboard::move(int dx,int dy) {
    row=std::max(0,std::min(4,row+dy));const int columns=row==4?4:10;
    column=std::max(0,std::min(columns-1,column+dx));
}
std::string Keyboard::label(int r,int c) const {
    static const char* rows[]={"qwertyuiop","asdfghjkl-","zxcvbnm.,?","1234567890"};
    if(r==4) {static const char* actions[]={"Space","Delete","Clear","Search"};return c>=0 && c<4?actions[c]:"";}
    if(r<0 || r>3 || c<0 || c>9) return "";
    char ch=rows[r][c];if(upper && ch>='a' && ch<='z') ch-=32;return std::string(1,ch);
}
void Keyboard::erase() {if(!value.empty()) value.pop_back();}
bool Keyboard::select() {
    if(row==4) {
        if(column==3) return value.find_first_not_of(' ')!=std::string::npos;
        if(column==1) {erase();return false;}if(column==2) {value.clear();return false;}
    }
    if(value.size()<96) value+=row==4?" ":label(row,column);return false;
}
size_t gridMove(size_t selected,size_t count,int dx,int dy,int columns) {
    if(!count || columns<1) return 0;
    if(selected>=count) selected=count-1;
    if(dx<0 && selected%(size_t)columns) --selected;
    if(dx>0 && selected%(size_t)columns<(size_t)columns-1 && selected+1<count) ++selected;
    if(dy<0 && selected>=(size_t)columns) selected-=columns;
    if(dy>0 && selected/(size_t)columns<(count-1)/(size_t)columns) selected=std::min(count-1,selected+(size_t)columns);
    return selected;
}
}
