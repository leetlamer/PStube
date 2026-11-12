#include "probe_graphics.h"
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <fstream>
int main() {
    for(auto dimensions:{std::pair<unsigned,unsigned>{1920,1080},{3840,2160}}) {
        unsigned w=dimensions.first,h=dimensions.second;size_t words=tiledWords(w,h);
        std::vector<unsigned char> seen(words);
        for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x) {
            size_t index=PS5_TilePixel(x,y,w);assert(index<words && !seen[index]);seen[index]=1;
        }
    }
    for(auto dimensions:{std::pair<unsigned,unsigned>{1920,1080},{3840,2160}}) {
        unsigned w=dimensions.first,h=dimensions.second;size_t words=tiledWords(w,h);
        std::vector<uint32_t> input((size_t)w*h),output(words+2,0x12345678);
        for(size_t i=0;i<input.size();++i) input[i]=(uint32_t)(i*2654435761u);
        tileFrameFast(input.data(),output.data()+1,w,h);
        assert(output.front()==0x12345678 && output.back()==0x12345678);
        for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x) assert(output[1+PS5_TilePixel(x,y,w)]==input[(size_t)y*w+x]);
    }
    size_t pixels=(size_t)probe::Width*probe::Height;
    std::vector<uint32_t> linear(pixels+2,0x12345678),tiled(tiledWords(probe::Width,probe::Height)+2,0x12345678);
    probe::draw(linear.data()+1,9000,true,false);
    // Long/off-screen text and rectangles must not overwrite the frame guards.
    probe::text(linear.data()+1,-50,-50,"PSTUBE PROBE",8);
    probe::rect(linear.data()+1,1910,1070,50,50,0xffffffff);
    probe::tileFrame(linear.data()+1,tiled.data()+1);
    assert(linear.front()==0x12345678 && linear.back()==0x12345678);
    assert(tiled.front()==0x12345678 && tiled.back()==0x12345678);
    for(unsigned y=0;y<probe::Height;++y) for(unsigned x=0;x<probe::Width;++x)
        assert(tiled[1+PS5_TilePixel(x,y,probe::Width)]==linear[1+(size_t)y*probe::Width+x]);
    probe::draw(linear.data()+1,9000,true,false);
    std::ofstream ppm("build/probe-preview.ppm",std::ios::binary);
    ppm<<"P6\n"<<probe::Width<<" "<<probe::Height<<"\n255\n";
    for(size_t i=1;i<=pixels;++i) {char rgb[3]={(char)linear[i],(char)(linear[i]>>8),(char)(linear[i]>>16)};ppm.write(rgb,3);}
    puts("PASS: 1080p/2160p tiling is bounded and collision-free; full frame roundtrip and clipped render guards");
}
