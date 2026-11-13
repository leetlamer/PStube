#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
namespace smarttube {
inline double carouselTarget(size_t cursor,size_t count) {return std::min((double)(cursor?cursor-1:0),(double)(count>4?count-4:0));}
struct CarouselMotion {
    double from=0,target=0,started=0;bool ready=false;
    double value(double now) const {double t=std::max(0.0,std::min(1.0,(now-started)/0.20));double ease=t*t*(3-2*t);return from+(target-from)*ease;}
    double step(double wanted,double now) {if(!ready) {ready=true;from=target=wanted;started=now;}else if(wanted!=target) {from=value(now);target=wanted;started=now;}return value(now);}
};
}
