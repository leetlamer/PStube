#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
namespace probe {
struct BufferPolicy {
    size_t capacity=8*1024*1024,startup=1,resume=1;
    static const char* label(unsigned level) {const char* names[]={"Low (5s)","Normal (30s)","High (50s)","Maximum (100s)"};return names[std::min(3u,level)];}
    static BufferPolicy media(unsigned bitrate,bool audio,bool live,unsigned level=1) {
        // Time-based startup/rebuffer hysteresis, bounded independently of duration.
        uint64_t rate=bitrate?bitrate:(audio?128000u:4000000u);
        auto bytes=[&](unsigned ms) {return (size_t)((rate*ms+7999)/8000);};
        level=std::min(3u,level);const unsigned durations[]={5000,30000,50000,100000},starts[]={1000,2500,4000,6000},refills[]={2000,5000,8000,12000};
        size_t ceiling=audio?2u*1024u*1024u:32u*1024u*1024u;
        BufferPolicy p;p.capacity=std::min(ceiling,std::max((size_t)256*1024,bytes(live?5000:durations[level])));
        p.startup=std::min(p.capacity/2,std::max((size_t)16384,bytes(live?1000:starts[level])));
        p.resume=std::min(p.capacity*3/4,std::max(p.startup,bytes(live?3000:refills[level])));return p;
    }
};
}
