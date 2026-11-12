#pragma once
#include <stdint.h>
#include <stddef.h>
#include "tile.h"
namespace probe {
constexpr unsigned Width=1920,Height=1080;
inline void pixel(uint32_t* pixels,int x,int y,uint32_t color,unsigned width=Width,unsigned height=Height) {
    if(x>=0 && y>=0 && x<(int)width && y<(int)height) pixels[(size_t)y*width+x]=color;
}
inline void rect(uint32_t* p,int x,int y,int w,int h,uint32_t c,unsigned width=Width,unsigned height=Height) {
    for(int row=0;row<h;++row) for(int col=0;col<w;++col) pixel(p,x+col,y+row,c,width,height);
}
inline void text(uint32_t* p,int x,int y,const char* str,int scale=4,unsigned width=Width,unsigned height=Height) {
    // Original 5x7 uppercase diagnostic font, one row per byte.
    static const uint8_t letters[26][7]={
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
    for(;*str;++str,x+=6*scale) {
        char ch=*str;if(ch>='a' && ch<='z') ch-=32;
        if(ch<'A' || ch>'Z') continue;
        for(int row=0;row<7;++row) for(int col=0;col<5;++col)
            if(letters[ch-'A'][row]&(16>>col)) rect(p,x+col*scale,y+row*scale,scale,scale,0xfff7f4ef,width,height);
    }
}
inline void draw(uint32_t* p,unsigned progress,bool pad,bool decoder) {
    for(size_t i=0;i<(size_t)Width*Height;++i) p[i]=0xff20120c;
    rect(p,64,60,1792,8,0xffff9123);
    text(p,80,110,"PSTUBE PS FIVE",9);
    text(p,80,230,"NATIVE PLATFORM TEST",6);
    text(p,80,340,pad?"CONTROLLER READY":"CONTROLLER UNAVAILABLE",4);
    text(p,80,410,decoder?"DECODER EXPORTS FOUND":"DECODER EXPORTS UNAVAILABLE",4);
    text(p,80,490,"HARDWARE VIDEO NOT TESTED",4);
    text(p,80,630,"CROSS PLAYS TEST TONE",4);
    text(p,80,700,"OPTIONS OR CIRCLE EXITS",4);
    text(p,80,770,"AUTO EXIT AFTER FIFTEEN SECONDS",4);
    rect(p,80,900,1760,24,0xff483224);
    rect(p,80,900,(int)(1760*(progress>15000?15000:progress)/15000),24,0xffff9123);
}
inline void tileFrame(const uint32_t* linear,uint32_t* tiled) {
    for(unsigned y=0;y<Height;++y) for(unsigned x=0;x<Width;++x)
        tiled[PS5_TilePixel(x,y,Width)]=linear[(size_t)y*Width+x];
}
}
