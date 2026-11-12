#include "parallel_tiler.h"
#include <assert.h>
#include <vector>
int main() {
    ParallelTiler tiler;
    for(unsigned cycle=0;cycle<3;++cycle) {
        assert(tiler.start());
        for(unsigned width:{1920u,3840u}) {
            unsigned height=width==1920?1080:2160;size_t words=tiledWords(width,height);
            std::vector<uint32_t> source((size_t)width*height),destination(words+2,0x7e123456);
            for(size_t i=0;i<source.size();++i) source[i]=(uint32_t)(i*2654435761u+cycle);
            tiler.frame(source.data(),destination.data()+1,width,height);
            assert(destination.front()==0x7e123456 && destination.back()==0x7e123456);
            for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x) assert(destination[1+PS5_TilePixel(x,y,width)]==source[(size_t)y*width+x]);
        }
        tiler.close();tiler.close();
    }
}
