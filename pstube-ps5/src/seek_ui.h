#pragma once
#include "shared/app_ui.h"
#include "seek_timeline.h"
#include "content_services.h"
namespace probe {
inline void drawSeekTimeline(smarttube::Canvas& canvas,unsigned w,int top,unsigned scale,double seconds,double total,bool focused,const std::vector<MuteRange>& markers) {
    if(total<=0) return;
                    std::string time=probe::playbackTime(seconds)+" / "+probe::playbackTime(total)+"    -"+probe::playbackTime(std::max(0.0,total-seconds))+" remaining";
                    canvas.text(time,50*scale,top+182*scale,23*scale,0xffeef6ff,w-100*scale);
                    canvas.rect(50*scale,top+225*scale,w-100*scale,10*scale,0xff365069);
                    unsigned filled=(unsigned)((w-100*scale)*std::max(0.0,std::min(1.0,seconds/total)));canvas.rect(50*scale,top+225*scale,filled,10*scale,focused?0xffeeeeff:0xff329dff);
                    const uint32_t markerColors[]={0xff23c552,0xfff2c94c,0xff56ccf2,0xff4361ee,0xffff69b4};const char* markerNames[]={"Sponsor","Self promotion","Intro","Outro","Interaction"};unsigned shown=0;
                    for(const auto& segment:markers) {unsigned index=std::min(segment.category,4u);double begin=std::max(0.0,std::min(1.0,segment.start/total)),end=std::max(begin,std::min(1.0,segment.end/total));unsigned x=(unsigned)((w-100*scale)*begin),right=(unsigned)((w-100*scale)*end);canvas.rect(50*scale+x,top+222*scale,std::max(2*scale,right-x),16*scale,markerColors[index]);shown|=1u<<index;}
                    canvas.rect(50*scale+std::min(filled,w-103*scale),top+218*scale,3*scale,24*scale,0xffeeeeff);
                    unsigned legend=0;for(unsigned index=0;index<5;++index) if(shown&(1u<<index)) {unsigned x=(50+legend*350)*scale;canvas.rect(x,top+250*scale,14*scale,14*scale,markerColors[index]);canvas.text(markerNames[index],x+22*scale,top+246*scale,17*scale,0xffeef6ff,320*scale);++legend;}
                    canvas.text(focused?"LEFT / RIGHT  Seek 10s    CROSS  Play here    CIRCLE  Cancel":"LEFT / RIGHT  Seek    UP  Controls    L1 / R1  Previous / next",50*scale,top+289*scale,20*scale,0xff9db4cc,w-100*scale);
}
}
