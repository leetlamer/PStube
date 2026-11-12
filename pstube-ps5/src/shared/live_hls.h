#pragma once
#include <string>
#include <stdint.h>
#include <vector>
namespace smarttube {
constexpr size_t LiveHlsMediaLimit=4*1024*1024;
struct LiveHlsVariant {std::string url,audioUrl,master;unsigned width=0,height=0,fps=0;};
struct LiveHlsDiagnostics {unsigned variants=0,accepted=0,codec=0,audio=0,range=0,resolution=0,fps=0,url=0;std::vector<std::string> details;};
struct LiveHlsMediaInfo {
    unsigned segments=0,targetSeconds=0,discontinuities=0;
    bool fragmented=false,byteRanges=false,ended=false;
    std::string latestSegment,initialization;
    uint64_t sequence=0;std::vector<std::string> segmentUrls;
};
bool selectLiveHls(const std::string& master,const std::string& base,unsigned height,unsigned fps,LiveHlsVariant& out,std::string& error,LiveHlsDiagnostics* diagnostics=nullptr,const std::string& language="original");
bool validateLiveHlsMedia(const std::string& media,const std::string& base,std::string& error,LiveHlsMediaInfo* info=nullptr);
const char* liveMediaSignature(const std::string& bytes);
}
