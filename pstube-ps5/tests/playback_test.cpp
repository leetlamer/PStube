#include "local_playback.h"
#include <assert.h>
#include <stdarg.h>
static unsigned frames=0;static bool cancel=false;static uint8_t output[1920*1080*4];
static void logger(const char* format,...) {va_list args;va_start(args,format);vprintf(format,args);va_end(args);puts("");}
static bool show(void*,const AVFrame* picture,SwsContext* scaler,unsigned,int,unsigned index,double* conversion,double* presentation) {
    if(cancel) return false;
    uint8_t* destination[]={output,nullptr,nullptr,nullptr};int stride[]={1920*4,0,0,0};
    double before=probe::benchmarkTime();assert(sws_scale(scaler,picture->data,picture->linesize,0,picture->height,destination,stride)==1080);
    assert(output[3]==255);assert(index==frames);++frames;*conversion=(probe::benchmarkTime()-before)*1000;*presentation=0;usleep(16667);return true;
}
int main() {
    assert(probe::playLocal(2160,6,show,nullptr,logger));assert(frames==600);
    cancel=true;assert(!probe::playLocal(1080,4,show,nullptr,logger));assert(!probe::playLocal(1234,4,show,nullptr,logger));
    probe::PlaybackTiming timing;timing.add(30);timing.add(10);timing.add(20);assert(timing.percentile(50)==20);assert(timing.percentile(100)==30);
}
