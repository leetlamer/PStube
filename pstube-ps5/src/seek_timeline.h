#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <cstdint>
namespace probe {
inline bool validRangeResponse(long status,int64_t requested,int64_t returned) {return (status==200 && requested==0) || (status==206 && returned==requested);}
inline double clampSeek(double seconds,double duration) {return std::max(0.0,std::min(std::isfinite(seconds)?seconds:0.0,std::max(0.0,duration-0.05)));}
inline std::string playbackTime(double seconds) {
    unsigned n=(unsigned)std::max(0.0,std::min(4320000.0,seconds));
    auto two=[](unsigned v) {return (v<10?"0":"")+std::to_string(v);};
    return n>=3600?std::to_string(n/3600)+":"+two(n/60%60)+":"+two(n%60):std::to_string(n/60)+":"+two(n%60);
}
}
