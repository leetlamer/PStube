#pragma once
#include "shared/json.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
namespace probe {
inline double serviceNumber(const smarttube::Json* value,double fallback=-1) {
    if(!value || value->type!=smarttube::Json::Number) return fallback;
    char* end=nullptr;double n=strtod(value->text.c_str(),&end);
    return end && !*end && std::isfinite(n)?n:fallback;
}
struct MuteRange {double start,end;unsigned category=0;};
inline std::vector<MuteRange> sponsorRanges(const smarttube::Json& json) {
    std::vector<MuteRange> out;if(json.type!=smarttube::Json::Array) return out;
    for(const auto& row:json.children) {
        if(row.string("category")!="sponsor" || (row.string("actionType")!="skip" && row.string("actionType")!="mute")) continue;
        auto* times=row.get("segment");if(!times || times->type!=smarttube::Json::Array || times->children.size()!=2) continue;
        double start=serviceNumber(&times->children[0]),end=serviceNumber(&times->children[1]);
        if(start>=0 && end>start && end<=43200 && out.size()<200) out.push_back({start,end});
    }return out;
}
inline const char* sponsorCategory(unsigned index) {static const char* names[]={"sponsor","selfpromo","intro","outro","interaction"};return index<5?names[index]:"";}
inline std::string sponsorQuery(unsigned mask) {std::string query="%5B";bool first=true;for(unsigned i=0;i<5;++i) if(mask&(1u<<i)) {if(!first) query+="%2C";query+="%22"+std::string(sponsorCategory(i))+"%22";first=false;}return query+"%5D";}
inline std::vector<MuteRange> sponsorSkips(const smarttube::Json& json,unsigned mask,unsigned minimum,double duration,bool merge=true) {
    std::vector<MuteRange> out;if(json.type!=smarttube::Json::Array) return out;
    for(const auto& row:json.children) {
        bool enabled=false;unsigned category=0;for(unsigned i=0;i<5;++i) if((mask&(1u<<i)) && row.string("category")==sponsorCategory(i)) {enabled=true;category=i;}
        if(!enabled || row.string("actionType")!="skip") continue;
        double reported=serviceNumber(row.get("videoDuration"),0);if(duration>0 && reported>0 && fabs(duration-reported)>1.0) continue;
        auto* times=row.get("segment");if(!times || times->type!=smarttube::Json::Array || times->children.size()!=2) continue;
        double start=serviceNumber(&times->children[0]),end=serviceNumber(&times->children[1]);
        if(start>=0 && end>start && end-start>=minimum && end<=43200 && (duration<=0 || end<=duration) && out.size()<200) out.push_back({start,end,category});
    }
    std::sort(out.begin(),out.end(),[](const MuteRange& a,const MuteRange& b) {return a.start<b.start;});if(!merge) return out;std::vector<MuteRange> merged;
    for(auto range:out) {if(!merged.empty() && range.start<=merged.back().end) merged.back().end=std::max(merged.back().end,range.end);else merged.push_back(range);}return merged;
}
inline void muteSponsorSamples(int16_t* samples,unsigned frames,double position,const std::vector<MuteRange>& ranges) {
    for(const auto& range:ranges) {
        double first=std::ceil((range.start-position)*48000-1e-6),last=std::ceil((range.end-position)*48000-1e-6);
        unsigned begin=(unsigned)std::max(0.0,std::min((double)frames,first)),end=(unsigned)std::max(0.0,std::min((double)frames,last));
        if(end>begin) std::fill(samples+begin*2,samples+end*2,0);
    }
}
struct Branding {std::string title;double timestamp=-1;};
inline Branding dearrowBranding(const smarttube::Json& json) {
    Branding result;
    auto trusted=[](const smarttube::Json& row) {auto* original=row.get("original");auto* locked=row.get("locked");return original && original->type==smarttube::Json::Bool && original->text=="false" && ((locked && locked->text=="true") || serviceNumber(row.get("votes"),-1)>=0);};
    if(auto* titles=json.get("titles")) for(const auto& row:titles->children) if(trusted(row)) {auto title=row.string("title");if(!title.empty() && title.size()<1024) {title.erase(std::remove(title.begin(),title.end(),'>'),title.end());result.title=title;break;}}
    if(auto* thumbnails=json.get("thumbnails")) for(const auto& row:thumbnails->children) if(trusted(row)) {double at=serviceNumber(row.get("timestamp"));if(at>=0 && at<=43200) {result.timestamp=at;break;}}
    return result;
}
}
