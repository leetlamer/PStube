#pragma once
#include <array>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <string>
#include <vector>
namespace smarttube {
// QR Model 2, version 5, byte mode, error correction L, fixed mask 0.
// 106-byte URL ceiling; four-module quiet zone is supplied by the renderer.
struct LoginQr {
    static constexpr int size=37;
    std::array<unsigned char,size*size> modules{};
    bool valid=false;
    explicit LoginQr(const std::string& text) {
        if(text.empty() || text.size()>106) return;
        std::vector<int> bits;
        const auto append=[&](unsigned value,int count){for(int i=count-1;i>=0;--i) bits.push_back((value>>i)&1);};
        append(4,4);append((unsigned)text.size(),8);for(unsigned char c:text) append(c,8);
        append(0,std::min(4,864-(int)bits.size()));while(bits.size()%8) bits.push_back(0);
        std::vector<unsigned char> words;
        for(size_t i=0;i<bits.size();i+=8) {unsigned v=0;for(int j=0;j<8;++j) v=v*2+bits[i+j];words.push_back((unsigned char)v);}
        for(unsigned i=0;words.size()<108;++i) words.push_back(i%2?0x11:0xec);
        const auto multiply=[](unsigned a,unsigned b){unsigned r=0;for(int i=0;i<8;++i) {if(b&1) r^=a;b>>=1;a<<=1;if(a&256) a^=0x11d;}return (unsigned char)r;};
        std::array<unsigned char,26> divisor{},remainder{};divisor[25]=1;unsigned root=1;
        for(int i=0;i<26;++i) {for(int j=0;j<26;++j) {divisor[j]=multiply(divisor[j],root);if(j<25) divisor[j]^=divisor[j+1];}root=multiply(root,2);}
        for(auto b:words) {const auto factor=b^remainder[0];for(int j=0;j<25;++j) remainder[j]=remainder[j+1];remainder[25]=0;for(int j=0;j<26;++j) remainder[j]^=multiply(divisor[j],factor);}
        words.insert(words.end(),remainder.begin(),remainder.end());
        std::array<bool,size*size> reserved{};
        const auto set=[&](int x,int y,bool black){if(x>=0 && y>=0 && x<size && y<size) {modules[y*size+x]=black;reserved[y*size+x]=true;}};
        for(int i=0;i<size;++i) {set(6,i,i%2==0);set(i,6,i%2==0);}
        for(auto center:std::array<std::pair<int,int>,3>{{{3,3},{size-4,3},{3,size-4}}})
            for(int dy=-4;dy<=4;++dy) for(int dx=-4;dx<=4;++dx) {const int d=std::max(abs(dx),abs(dy));set(center.first+dx,center.second+dy,d!=2 && d!=4);}
        for(int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx) set(30+dx,30+dy,std::max(abs(dx),abs(dy))!=1);
        const unsigned format=0x77c4; // BCH-protected L/mask0 format.
        for(int i=0;i<6;++i) set(8,i,(format>>i)&1);
        set(8,7,(format>>6)&1);set(8,8,(format>>7)&1);set(7,8,(format>>8)&1);
        for(int i=9;i<15;++i) set(14-i,8,(format>>i)&1);
        for(int i=0;i<8;++i) set(size-1-i,8,(format>>i)&1);
        for(int i=8;i<15;++i) set(8,size-15+i,(format>>i)&1);
        set(8,size-8,true);
        size_t bit=0;
        for(int right=size-1;right>=1;right-=2) {
            if(right==6) right=5;
            for(int vert=0;vert<size;++vert) {const int y=((right+1)&2)==0?size-1-vert:vert;
                for(int j=0;j<2;++j) {const int x=right-j;if(reserved[y*size+x]) continue;
                    bool black=bit<words.size()*8 && ((words[bit/8]>>(7-bit%8))&1);++bit;
                    modules[y*size+x]=black^((x+y)%2==0);
                }
            }
        }
        valid=true;
    }
};
}
