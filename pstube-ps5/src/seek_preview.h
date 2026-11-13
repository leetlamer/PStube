#pragma once
#include "shared/core.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
namespace probe {
struct SeekPreviewFrame {std::string url;smarttube::Rect crop;unsigned index=0;};
struct SeekStoryboard {
    std::string base,name,signature;unsigned width=0,height=0,columns=0,rows=0,interval=0,level=0;
    static std::vector<std::string> split(const std::string& value,char separator) {std::vector<std::string> out;size_t start=0;for(size_t i=0;i<=value.size();++i) if(i==value.size() || value[i]==separator) {out.push_back(value.substr(start,i-start));start=i+1;}return out;}
    static unsigned number(const std::string& value) {if(value.empty() || value.size()>8 || value.find_first_not_of("0123456789")!=std::string::npos) return 0;return (unsigned)strtoul(value.c_str(),nullptr,10);}
    static void replace(std::string& value,const std::string& key,const std::string& text) {size_t at=0;while((at=value.find(key,at))!=std::string::npos) {value.replace(at,key.size(),text);at+=text.size();}}
    static SeekStoryboard parse(const std::string& spec) {
        SeekStoryboard out;if(spec.size()>16384 || spec.find_first_of("\r\n")!=std::string::npos) return out;auto sections=split(spec,'|');if(sections.size()<2 || sections.size()>12 || sections[0].compare(0,23,"https://i.ytimg.com/sb/")!=0) return out;
        for(size_t i=1;i<sections.size();++i) {auto p=split(sections[i],'#');if(p.size()!=8) continue;unsigned w=number(p[0]),h=number(p[1]),cols=number(p[3]),rows=number(p[4]),ms=number(p[5]);
            if(!w || !h || w>640 || h>360 || !cols || !rows || cols>20 || rows>20 || w*cols>2048 || h*rows>2048 || ms<100 || ms>60000 || p[6].size()>100 || p[7].size()>512 || p[7].find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-=.$~%")!=std::string::npos) continue;
            if(w>=out.width) {out.base=sections[0];out.width=w;out.height=h;out.columns=cols;out.rows=rows;out.interval=ms;out.name=p[6];out.signature=p[7];out.level=(unsigned)i-1;}
        }return out;
    }
    bool valid() const {return interval && columns && rows && !base.empty();}
    SeekPreviewFrame at(double seconds) const {
        SeekPreviewFrame frame;if(!valid() || !std::isfinite(seconds) || seconds<0 || seconds>43200) return frame;frame.index=(unsigned)(seconds*1000/interval);unsigned cell=frame.index%(columns*rows);frame.crop={(int)(cell%columns*width),(int)(cell/columns*height),(int)width,(int)height};
        frame.url=base;replace(frame.url,"$L",std::to_string(level));replace(frame.url,"$N",name);replace(frame.url,"$M",std::to_string(frame.index/(columns*rows)));if(!signature.empty()) frame.url+=(frame.url.find('?')==std::string::npos?"?":"&")+std::string("sigh=")+signature;return frame;
    }
};
}
