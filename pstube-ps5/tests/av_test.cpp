#include "av_playback.h"
#include "adaptive_audio.h"
#include <assert.h>
#include <stdarg.h>
static unsigned blocks=0,pictures=0;static uint8_t pixels[1920*1080*4];
static void log(const char* format,...) {va_list args;va_start(args,format);vprintf(format,args);va_end(args);puts("");}
static bool audio(void*,const int16_t*) {++blocks;usleep(21333);return true;}
static bool video(void*,const AVFrame* frame,SwsContext* scaler,unsigned,int,unsigned,double* conversion,double* presentation) {
    uint8_t* out[]={pixels};int strides[]={1920*4};assert(sws_scale(scaler,frame->data,frame->linesize,0,frame->height,out,strides)==1080);++pictures;*conversion=0;*presentation=0;usleep(1000);return true;
}
int main() {
    auto* queue=new probe::PcmQueue(audio,nullptr,log);assert(queue->start());assert(probe::playAv("assets/av-1080.webm",video,nullptr,*queue,log));queue->finish(true);assert(pictures==720);assert(blocks>=562 && blocks<=565);delete queue;
    FILE* videoFile=fopen("assets/av-1080.webm","rb");FILE* audioFile=fopen("assets/av-1080.webm","rb");assert(videoFile && audioFile);
    AVIOContext* vi=avio_alloc_context(static_cast<uint8_t*>(av_malloc(65536)),65536,0,videoFile,probe::avFileRead,nullptr,probe::avFileSeek);
    AVIOContext* ai=avio_alloc_context(static_cast<uint8_t*>(av_malloc(65536)),65536,0,audioFile,probe::avFileRead,nullptr,probe::avFileSeek);assert(vi && ai);
    queue=new probe::PcmQueue(audio,nullptr,log);queue->pause(true);assert(queue->start());auto* decoder=new probe::AdaptiveAudio(ai,*queue,log);assert(decoder->start());unsigned before=pictures;
    assert(probe::playAv("ignored",video,nullptr,*queue,log,vi,0,true));decoder->join();queue->finish(true);assert(pictures-before==720);delete decoder;delete queue;
    av_freep(&vi->buffer);avio_context_free(&vi);av_freep(&ai->buffer);avio_context_free(&ai);fclose(videoFile);fclose(audioFile);
    // Decode from an earlier keyframe, but present/submit only the requested timeline.
    videoFile=fopen("assets/av-1080.webm","rb");audioFile=fopen("assets/av-1080.webm","rb");assert(videoFile && audioFile);
    vi=avio_alloc_context(static_cast<uint8_t*>(av_malloc(65536)),65536,0,videoFile,probe::avFileRead,nullptr,probe::avFileSeek);
    ai=avio_alloc_context(static_cast<uint8_t*>(av_malloc(65536)),65536,0,audioFile,probe::avFileRead,nullptr,probe::avFileSeek);
    queue=new probe::PcmQueue(audio,nullptr,log);queue->pause(true);assert(queue->start());decoder=new probe::AdaptiveAudio(ai,*queue,log,8.0);assert(decoder->start());double position=0;probe::VideoStreamMetrics metrics;before=pictures;
    assert(!probe::playAv("ignored",video,nullptr,*queue,log,vi,60,true,1920,1080,false,false,&position,8.0,&metrics));assert(metrics.width==1920 && metrics.height==1080 && metrics.fps==60 && std::string(metrics.codec)=="vp9");
    decoder->cancel();queue->finish(false);decoder->join();double origin=0;assert(queue->getMediaOrigin(origin) && origin>=8 && origin<8.1);assert(pictures-before==60 && position>=8900 && position<9100);delete decoder;delete queue;
    av_freep(&vi->buffer);avio_context_free(&vi);av_freep(&ai->buffer);avio_context_free(&ai);fclose(videoFile);fclose(audioFile);
    queue=new probe::PcmQueue(audio,nullptr,log);assert(queue->start());assert(!probe::playAv("assets/av-1080.webm",video,nullptr,*queue,log,nullptr,3));queue->finish(false);delete queue;
}
