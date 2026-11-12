#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace probe {
inline uint32_t little32(const uint8_t* p) {return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
inline unsigned little16(const uint8_t* p) {return p[0]|(unsigned)p[1]<<8;}
struct IvfView {
    const uint8_t* bytes=nullptr;size_t size=0,cursor=32;unsigned width=0,height=0,count=0;
    bool open(const uint8_t* data,size_t length) {
        bytes=nullptr;size=0;cursor=32;
        if(!data || length<32 || length>16*1024*1024 || memcmp(data,"DKIF",4) || little16(data+4)!=0 || little16(data+6)!=32 || memcmp(data+8,"VP90",4)) return false;
        width=little16(data+12);height=little16(data+14);count=little32(data+24);
        if(width<16 || width>3840 || height<16 || height>2160 || !count || count>240 || little32(data+16)!=60 || little32(data+20)!=1) return false;
        size_t at=32;
        for(unsigned i=0;i<count;++i) {
            if(length-at<12) return false;uint32_t n=little32(data+at);at+=12;
            if(!n || n>4*1024*1024 || n>length-at) return false;at+=n;
        }
        if(at!=length) return false;bytes=data;size=length;cursor=32;return true;
    }
    bool next(const uint8_t*& frame,size_t& length) {
        if(!bytes || cursor>=size) return false;
        length=little32(bytes+cursor);cursor+=12;frame=bytes+cursor;cursor+=length;return true;
    }
};
}
