#include "pcm_queue.h"
#include "buffer_policy.h"
#include <assert.h>
static void log(const char*,...) {}
static bool sink(void*,const int16_t*) {return true;}
int main() {
    auto* pcm=new probe::PcmQueue(sink,nullptr,log);alignas(16) int16_t data[2048]{};pcm->pause(true);assert(pcm->start());assert(pcm->write(data,1024));usleep(30000);assert(pcm->firstOutput()==0);
    double resumed=probe::benchmarkTime();pcm->pause(false);for(int n=0;n<100 && !pcm->firstOutput();++n) usleep(1000);double anchor=pcm->firstOutput();assert(anchor>=resumed && anchor-resumed<0.05);
    pcm->pause(true);usleep(25000);pcm->pause(false);assert(pcm->firstOutput()-anchor>=0.025);double beforeGap=pcm->firstOutput();usleep(50000);assert(pcm->write(data,1024));usleep(5000);assert(pcm->firstOutput()-beforeGap>=0.045);
    pcm->pause(true);pcm->buffering(true);pcm->buffering(false);double held=pcm->firstOutput();assert(pcm->write(data,1024));usleep(25000);assert(pcm->firstOutput()==held);pcm->pause(false);
    pcm->buffering(true);pcm->pause(true);pcm->pause(false);held=pcm->firstOutput();usleep(25000);assert(pcm->firstOutput()==held);pcm->buffering(false);assert(pcm->firstOutput()-held>=0.025);
    for(unsigned rate:{0u,128000u,4000000u,80000000u}) for(bool audio:{false,true}) for(bool live:{false,true}) for(unsigned level=0;level<4;++level) {auto p=probe::BufferPolicy::media(rate,audio,live,level);assert(p.capacity<=(audio?2u:32u)*1024*1024);assert(p.startup>0 && p.startup<=p.resume && p.resume<p.capacity);}
    auto vod=probe::BufferPolicy::media(4000000,false,false),live=probe::BufferPolicy::media(4000000,false,true);assert(vod.startup==1250000 && vod.resume==2500000 && live.startup==500000 && live.resume==1500000);
    pcm->mediaOrigin(12.5);pcm->mediaOrigin(20);double origin=0;assert(pcm->getMediaOrigin(origin) && origin==12.5);pcm->finish(true);delete pcm;
}
