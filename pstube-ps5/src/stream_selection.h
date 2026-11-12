#pragma once
#include "shared/core.h"
namespace probe {
struct AdaptiveChoice {int video=-1,audio=-1;};
inline AdaptiveChoice adaptiveChoice(const smarttube::PlayerInfo& info,int maxHeight,int maxFps) {
    AdaptiveChoice choice;int videoScore=-1,audioScore=-1;
    for(size_t i=0;i<info.streams.size();++i) {
        const auto& track=info.streams[i];if(!track.adaptive || track.requiresSignature || track.url.compare(0,8,"https://")) continue;
        bool vp9=track.mime.find("vp9")!=std::string::npos || track.mime.find("vp09")!=std::string::npos;
        bool avc=track.mime.find("avc1")!=std::string::npos;
        bool hdr=track.mime.find("vp09.02")!=std::string::npos || track.mime.find("vp9.2")!=std::string::npos;
        if((vp9 || avc) && !hdr && track.height<=maxHeight && track.height>0 && track.width>0 && track.width<=3840 && track.fps>0 && track.fps<=maxFps) {
            int score=track.height*100+track.fps+(vp9?1000000:0);
            if(score>videoScore) {videoScore=score;choice.video=(int)i;}
        }
        bool opus=track.mime.find("opus")!=std::string::npos,aac=track.mime.find("mp4a")!=std::string::npos;
        if((opus || aac) && !track.autoDubbed) {
            int score=track.bitrate+(opus?10000000:0)+(track.audioName.find("original")!=std::string::npos?20000000:0);
            if(score>audioScore) {audioScore=score;choice.audio=(int)i;}
        }
    }return choice;
}
}
