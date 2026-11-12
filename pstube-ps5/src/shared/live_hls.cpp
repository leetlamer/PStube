#include "live_hls.h"
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <vector>
namespace smarttube {
namespace {
bool url(const std::string& s) {
    if(s.find('"')!=std::string::npos || s.find('\0')!=std::string::npos || s.find('\\')!=std::string::npos || s.size()>16384 || s.compare(0,8,"https://") || s.find_first_of("\r\n\t ")!=std::string::npos) return false;
    const auto end=s.find_first_of("/?#",8);const auto host=s.substr(8,end==std::string::npos?end:end-8);
    return !host.empty() && host.find('@')==std::string::npos;
}
std::string resolve(const std::string& base,const std::string& part) {
    if(url(part)) return part;
    if(!url(base) || part.empty() || part.find(":")!=std::string::npos || part.compare(0,2,"//")==0 || part.find("..")!=std::string::npos || part[0]=='?' || part[0]=='#') return {};
    const auto hostEnd=base.find('/',8);const auto origin=base.substr(0,hostEnd);
    auto clean=base.substr(0,base.find_first_of("?#"));
    auto directory=clean.find('/',8)==std::string::npos?clean+"/":clean.substr(0,clean.rfind('/')+1);
    auto result=part[0]=='/'?origin+part:directory+part;
    return url(result)?result:std::string();
}
std::string attr(const std::string& line,const std::string& key) {
    size_t at=0;
    while(at<line.size()) {
        auto equals=line.find('=',at);if(equals==std::string::npos) break;
        const auto name=line.substr(at,equals-at);size_t next=equals+1;std::string value;
        if(next<line.size() && line[next]=='"') {const auto end=line.find('"',next+1);if(end==std::string::npos) return {};value=line.substr(next+1,end-next-1);next=end+1;}
        else {const auto end=line.find(',',next);value=line.substr(next,end==std::string::npos?end:end-next);next=end==std::string::npos?line.size():end;}
        if(name==key) return value;
        if(next<line.size() && line[next]!=',') return {};
        at=next+1;
    }
    return {};
}
bool avcAac(const std::string& s) {
    bool avc=false,aac=false;unsigned count=0;size_t at=0;
    while(at<s.size()) {const auto end=s.find(',',at);auto token=s.substr(at,end==std::string::npos?end:end-at);const auto first=token.find_first_not_of(' '),last=token.find_last_not_of(' ');if(first==std::string::npos) return false;token=token.substr(first,last-first+1);
        if(token.size()==11 && token.compare(0,5,"avc1.")==0 && token.find_first_not_of("0123456789abcdefABCDEF",5)==std::string::npos && !avc && (token.substr(5,2)=="42" || token.substr(5,2)=="4d" || token.substr(5,2)=="4D" || token.substr(5,2)=="64") && strtoul(token.substr(9,2).c_str(),nullptr,16)<=42) avc=true;
        else if(token=="mp4a.40.2" && !aac) aac=true;else return false;
        ++count;if(end==std::string::npos) break;at=end+1;
    }
    return count==2 && avc && aac;
}
unsigned number(const std::string& s) {if(s.empty() || s.size()>9 || s.find_first_not_of("0123456789")!=std::string::npos) return 0;return (unsigned)strtoul(s.c_str(),nullptr,10);}
bool lines(const std::string& data,std::vector<std::string>& out,size_t limit=512*1024) {
    if(data.empty() || data.size()>limit || data.compare(0,7,"#EXTM3U") || data.find('\0')!=std::string::npos) return false;
    size_t at=0;
    while(at<data.size()) {const auto end=data.find('\n',at);auto line=data.substr(at,end==std::string::npos?end:end-at);if(!line.empty() && line.back()=='\r') line.pop_back();if(line.size()>16384 || out.size()>=32768) return false;out.push_back(line);if(end==std::string::npos) break;at=end+1;}
    return !out.empty() && out[0]=="#EXTM3U";
}
}
bool selectLiveHls(const std::string& master,const std::string& base,unsigned height,unsigned fps,LiveHlsVariant& out,std::string& error,LiveHlsDiagnostics* diagnostics,const std::string& language) {
    out={};error.clear();LiveHlsDiagnostics local;auto& stats=diagnostics?*diagnostics:local;stats={};
    std::vector<std::string> list;if(!url(base) || !lines(master,list)) {error="Invalid live HLS master playlist";return false;}
    height=std::min(height,1080u);fps=std::min(fps,60u);std::string pending;
    for(const auto& line:list) {
        if(line.compare(0,19,"#EXT-X-SESSION-KEY:")==0) {error="Encrypted live HLS is not supported";return false;}
        if(line.compare(0,18,"#EXT-X-STREAM-INF:")==0) {pending=line.substr(18);continue;}
        if(line.empty() || line[0]=='#' || pending.empty()) continue;
        const auto attributes=pending;pending.clear();++stats.variants;
        const auto codecs=attr(attributes,"CODECS"),resolution=attr(attributes,"RESOLUTION"),group=attr(attributes,"AUDIO");
        const auto split=resolution.find('x');const unsigned w=split==std::string::npos?0:number(resolution.substr(0,split)),h=split==std::string::npos?0:number(resolution.substr(split+1));
        const auto rate=attr(attributes,"FRAME-RATE");char* rateEnd=nullptr;const double f=rate.empty()?0:strtod(rate.c_str(),&rateEnd);
        std::string reason,audioUrl;int bestAudio=-1;const auto bandwidth=number(attr(attributes,"BANDWIDTH"));
        if(!avcAac(codecs)) {++stats.codec;reason="codec/profile/level";}
        else if(!attr(attributes,"VIDEO-RANGE").empty() && attr(attributes,"VIDEO-RANGE")!="SDR") {++stats.range;reason="HDR";}
        else if(!w || w>1920 || !h || h>height) {++stats.resolution;reason="resolution";}
        else if(!isfinite(f) || f<0 || f>fps || (!rate.empty() && (!rateEnd || *rateEnd))) {++stats.fps;reason="fps";}
        if(reason.empty() && !group.empty()) {
            for(const auto& entry:list) if(entry.compare(0,13,"#EXT-X-MEDIA:")==0) {
                const auto attributes=entry.substr(13);
                if(attr(attributes,"TYPE")!="AUDIO" || attr(attributes,"GROUP-ID")!=group) continue;
                const auto uri=attr(attributes,"URI");const auto resolved=uri.empty()?std::string():resolve(base,uri);
                if(!uri.empty() && resolved.empty()) continue;
                const auto channels=attr(attributes,"CHANNELS");if(!channels.empty() && channels!="1" && channels!="2") continue;
                const auto lang=attr(attributes,"LANGUAGE");
                const int score=(attr(attributes,"DEFAULT")=="YES"?4:0)+(attr(attributes,"AUTOSELECT")=="YES"?1:0)+(language=="en" && (lang=="en" || lang.compare(0,3,"en-")==0)?8:0);
                if(score>bestAudio) {bestAudio=score;audioUrl=resolved;}
            }
            if(bestAudio<0) {++stats.audio;reason="audio-group";}
        }
        if(reason.empty() && !audioUrl.empty() && (!bandwidth || bandwidth>100000000)) {++stats.audio;reason="audio-bandwidth";}
        const auto target=resolve(base,line);
        if(reason.empty() && target.empty()) {++stats.url;reason="url";}
        if(stats.details.size()<16) {
            const bool safe=codecs.size()<=96 && codecs.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789., -")==std::string::npos;
            stats.details.push_back("codec="+(safe?codecs:std::string("invalid-or-oversized"))+" size="+std::to_string(w)+"x"+std::to_string(h)+" fps="+(isfinite(f) && f>=0 && f<1000?std::to_string(f):std::string("invalid"))+" audio_group="+(group.empty()?"0":"1")+" result="+(reason.empty()?"accepted":reason));
        }
        if(!reason.empty()) continue;++stats.accepted;
        if(out.url.empty() || h>out.height || (h==out.height && f>out.fps)) {
            out={};out.url=target;out.audioUrl=audioUrl;out.width=w;out.height=h;out.fps=(unsigned)ceil(f);
            if(!audioUrl.empty()) {
                // A small local master preserves the relationship between the
                // selected video and external audio. Signed URLs are unchanged.
                out.master="#EXTM3U\n#EXT-X-VERSION:6\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"audio\",NAME=\"Audio\",DEFAULT=YES,AUTOSELECT=YES,URI=\""+audioUrl+"\"\n#EXT-X-STREAM-INF:BANDWIDTH="+std::to_string(bandwidth)+",CODECS=\""+codecs+"\",RESOLUTION="+std::to_string(w)+"x"+std::to_string(h)+",AUDIO=\"audio\"\n"+target+"\n";
            }
        }
    }
    if(out.url.empty()) {error="Live HLS variants rejected: codec="+std::to_string(stats.codec)+", audio="+std::to_string(stats.audio)+", size="+std::to_string(stats.resolution)+", fps="+std::to_string(stats.fps)+", HDR="+std::to_string(stats.range);return false;}
    return true;
}
bool validateLiveHlsMedia(const std::string& media,const std::string& base,std::string& error,LiveHlsMediaInfo* info) {
    error.clear();if(info) *info={};LiveHlsMediaInfo result;
    std::vector<std::string> list;if(!url(base) || !lines(media,list,LiveHlsMediaLimit)) {error="Invalid live HLS media playlist";return false;}
    unsigned segments=0;bool duration=false,target=false;
    for(const auto& line:list) {
        if(line.compare(0,11,"#EXT-X-KEY:")==0 && attr(line.substr(11),"METHOD")!="NONE") {error="Encrypted live HLS is not supported";return false;}
        if(line.compare(0,19,"#EXT-X-SESSION-KEY:")==0) {error="Encrypted live HLS is not supported";return false;}
        if(line.compare(0,11,"#EXT-X-MAP:")==0 && resolve(base,attr(line.substr(11),"URI")).empty()) {error="Invalid live HLS initialization URL";return false;}
        if(line.compare(0,11,"#EXT-X-MAP:")==0) {result.fragmented=true;result.initialization=resolve(base,attr(line.substr(11),"URI"));if(!attr(line.substr(11),"BYTERANGE").empty()) result.byteRanges=true;}
        if(line.compare(0,17,"#EXT-X-BYTERANGE:")==0) result.byteRanges=true;
        if(line=="#EXT-X-DISCONTINUITY") ++result.discontinuities;
        if(line.compare(0,22,"#EXT-X-MEDIA-SEQUENCE:")==0) {const auto value=line.substr(22);if(value.empty() || value.size()>15 || value.find_first_not_of("0123456789")!=std::string::npos) {error="Invalid live HLS media sequence";return false;}result.sequence=strtoull(value.c_str(),nullptr,10);}
        if(line=="#EXT-X-ENDLIST") result.ended=true;
        if(line.compare(0,22,"#EXT-X-TARGETDURATION:")==0) {const auto n=number(line.substr(22));target=n>0 && n<=120;}
        if(line.compare(0,8,"#EXTINF:")==0) duration=true;
        if(line.compare(0,22,"#EXT-X-TARGETDURATION:")==0) result.targetSeconds=number(line.substr(22));
        if(!line.empty() && line[0]!='#') {if(!duration || resolve(base,line).empty() || ++segments>8192) {error="Invalid live HLS segment layout";return false;}result.latestSegment=resolve(base,line);if(info) result.segmentUrls.push_back(result.latestSegment);duration=false;}
    }
    if(!segments || !target) {error="Live HLS has no complete media segments";return false;}
    result.segments=segments;if(info) *info=std::move(result);return true;
}
const char* liveMediaSignature(const std::string& bytes) {
    const auto* b=reinterpret_cast<const unsigned char*>(bytes.data());
    if(bytes.size()>=377 && b[0]==0x47 && b[188]==0x47 && b[376]==0x47) return "mpeg-ts";
    if(bytes.size()>=8 && (bytes.compare(4,4,"ftyp")==0 || bytes.compare(4,4,"styp")==0 || bytes.compare(4,4,"moof")==0 || bytes.compare(4,4,"sidx")==0)) return "iso-bmff";
    if(bytes.size()>=3 && bytes.compare(0,3,"ID3")==0) return "id3-prefixed";
    if(bytes.size()>=2 && b[0]==0xff && (b[1]&0xf6)==0xf0) return "aac-adts";
    return bytes.empty()?"empty":"unknown";
}
}
