/*
  Simple DirectMedia Layer
  Copyright (C) 2026 John Törnblom <john.tornblom@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// Altered adaptation: scalar reference tiling only, no SDL dependency.
#pragma once
#include <stdint.h>
#include <stddef.h>
constexpr uint32_t PS5_TILE_WIDTH=512,PS5_TILE_HEIGHT=128,PS5_TILE_SIZE=512*128;
static constexpr uint32_t PS5_TileOffset(uint32_t x, uint32_t y)
{
    return (((x) & 1u) << 0 |
            ((x >>  1) & 1u) << 1 |
            ((y >>  0) & 1u) << 2 |
            ((y >>  1) & 1u) << 3 |
            ((y >>  2) & 1u) << 4 |
            ((x >>  2) & 1u) << 5 |
            (((x >> 3) ^ (y >> 3)) & 1u) << 6 |
            (((x >> 4) ^ (y >> 4)) & 1u) << 7 |
            (((x >> 6) ^ (y >> 5)) & 1u) << 8 |
            (((x >> 5) ^ (y >> 6)) & 1u) << 9 |
            ((y >>  3) & 1u) << 10 |
            ((x >>  4) & 1u) << 11 |
            ((y >>  6) & 1u) << 12 |
            ((x >>  6) & 1u) << 13 |
            ((x >>  7) & 1u) << 14 |
            ((x >>  8) & 1u) << 15);
}

static uint32_t PS5_TilePixel(uint32_t x, uint32_t y, uint32_t width)
{
    return ((x / PS5_TILE_WIDTH) * PS5_TILE_SIZE +
            (y / PS5_TILE_HEIGHT) * (PS5_TILE_HEIGHT * width) +
            ((PS5_TileOffset((x % PS5_TILE_WIDTH), 0) ^
              PS5_TileOffset(0, (y % PS5_TILE_HEIGHT)))));
}


inline size_t tiledWords(uint32_t width,uint32_t height) {
    return ((height-1)/128)*(128*width)+((width-1)/512)*65536+65536;
}

// PStube CPU fast path: each four-pixel group is contiguous in the verified
// SDL mapping. Lookup tables remove per-pixel address bit mixing.
struct TileLookup {
    uint32_t x[128]{},y[128]{};
    constexpr TileLookup() {for(unsigned i=0;i<128;++i) {x[i]=PS5_TileOffset(i*4,0);y[i]=PS5_TileOffset(0,i);}}
};
inline constexpr TileLookup TileOffsets{};
inline void tileFrameRows(const uint32_t* source,uint32_t* destination,unsigned width,unsigned firstRow,unsigned lastRow) {
    for(unsigned y=firstRow;y<lastRow;++y) {
        size_t base=(size_t)(y/128)*(128*width);uint32_t offsetY=TileOffsets.y[y%128];
        for(unsigned tileX=0;tileX<width;tileX+=512) {
            unsigned count=width-tileX;if(count>512) count=512;
            size_t tile=base+(tileX/512)*65536;
            for(unsigned x=0;x<count;x+=4) {
                size_t index=tile+(TileOffsets.x[x/4]^offsetY);
                if(x+4<=count) __builtin_memcpy(destination+index,source+(size_t)y*width+tileX+x,16);
                else for(unsigned lane=0;lane<4 && x+lane<count;++lane) destination[index+lane]=source[(size_t)y*width+tileX+x+lane];
            }
        }
    }
}

inline void tileFrameFast(const uint32_t* source,uint32_t* destination,unsigned width,unsigned height) {tileFrameRows(source,destination,width,0,height);}
