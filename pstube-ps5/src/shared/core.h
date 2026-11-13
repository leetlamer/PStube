#pragma once
#include "json.h"
#include <stdint.h>

namespace smarttube {
struct Rect { int x=0,y=0,w=0,h=0; };
struct DisplayLayout {
    int outputWidth=1920,outputHeight=1080;
    Rect safe, sidebar, content;
    int columns=0,cardWidth=0,cardHeight=0,gap=0,fontPixels=0;
    float scale=1;
};
DisplayLayout layout(int outputWidth,int outputHeight,float overscan=0.05f);
Rect fitVideo(int videoWidth,int videoHeight,Rect viewport);
struct Video { std::string id,title,channel,thumbnail;std::string badge{};unsigned resumeMs=0;std::string browseId{},browseParams{};std::string views{},published{},likes{},dislikes{},subscribers{}; };
std::vector<Video> catalog(const Json& response);
struct CatalogStats { unsigned videoIds=0,watchEndpoints=0,tileHeaders=0,overlayMetadata=0,lockups=0; };
std::vector<Video> channelCatalog(const Json& response);
std::vector<Video> playlistCatalog(const Json& response);
std::string catalogContinuation(const Json& response);
CatalogStats catalogStats(const Json& response);
struct DecoderCaps {
    int maxWidth=1920,maxHeight=1080,maxFps=60;
    bool avc=false,aac=false; // Set only after actual backend capability verification.
};
struct Stream {
    int itag=0,width=0,height=0,fps=0,bitrate=0;
    std::string mime,url,cipher;
    bool adaptive=false,requiresSignature=false;
    std::string audioId{},audioLanguage{},audioName{};bool audioDefault=false,autoDubbed=false;
};
struct PlayerInfo {
    std::string status,reason,title,videoId,hls,dash,sabr;
    bool live=false;
    unsigned durationMs=0;
    std::vector<Stream> streams;
};
PlayerInfo playerInfo(const Json& response);
enum class PlaybackRequirement { DirectMuxedAvc, Unplayable, DecoderUnverified, SignatureOrTokenResolver, DashDemuxer, SabrTransport, NoCompatibleStream };
struct PlaybackChoice { PlaybackRequirement requirement; int index=-1; };
PlaybackChoice chooseStream(const PlayerInfo& info,DecoderCaps caps,int outputHeight);
struct Request { std::string url,body,userAgent,clientName,clientVersion;std::string visitorData{},clientNumber{};std::string contentType="application/json",authorization{};bool httpGet=false;std::string accept="application/json"; };
Request searchRequest(const std::string& query,const std::string& client,const std::string& version);
constexpr unsigned AnonymousProfiles=3;
const char* anonymousProfileName(unsigned index);
Request anonymousPlayerRequest(const std::string& videoId,unsigned index,const std::string& visitorData);
Request playerRequest(const std::string& videoId,const std::string& client,const std::string& version);
}
