#include "pcm_queue.h"
#include <assert.h>
static void log(const char*,...) {}
static bool sink(void*,const int16_t*) {return true;}
int main() {
    auto* pcm=new probe::PcmQueue(sink,nullptr,log);alignas(16) int16_t data[2048]{};pcm->pause(true);assert(pcm->start());assert(pcm->write(data,1024));usleep(30000);assert(pcm->firstOutput()==0);
    double resumed=probe::benchmarkTime();pcm->pause(false);for(int n=0;n<100 && !pcm->firstOutput();++n) usleep(1000);double anchor=pcm->firstOutput();assert(anchor>=resumed && anchor-resumed<0.05);
    pcm->pause(true);usleep(25000);pcm->pause(false);assert(pcm->firstOutput()-anchor>=0.025);double beforeGap=pcm->firstOutput();usleep(50000);assert(pcm->write(data,1024));usleep(5000);assert(pcm->firstOutput()-beforeGap>=0.045);
    pcm->mediaOrigin(12.5);pcm->mediaOrigin(20);double origin=0;assert(pcm->getMediaOrigin(origin) && origin==12.5);pcm->finish(true);delete pcm;
}
