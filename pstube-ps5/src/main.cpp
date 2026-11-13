#include "ps5_abi.h"
#include "probe_graphics.h"
#include "parallel_tiler.h"
#ifdef PSTUBE_SOFTWARE_BENCH
#include "software_benchmark.h"
#endif
#ifdef PSTUBE_LOCAL_PLAYBACK
#include "local_playback.h"
#ifdef PSTUBE_AV_PLAYBACK
#include "av_playback.h"
#include "network_probe.h"
#include "curl_transport.h"
#include "adaptive_audio.h"
#ifdef PSTUBE_PROTOTYPE
#include "shared/app_ui.h"
#include "stream_selection.h"
#include "beta_account.h"
#include "thumbnail_loader.h"
#include "player_features.h"
#include "seek_timeline.h"
#include "seek_ui.h"
#include "seek_preview.h"
#include "document_job.h"
#include "network_preferences.h"
#include "network_ui.h"
#include "shared/login_qr.h"
#include <functional>
#endif
#endif
#endif
#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <poll.h>
#include "remote_keys.h"

namespace {
FILE* logFile=nullptr;
const char* logPath="stdout only";
void logLine(const char* format,...) {
    char line[1024];va_list args;va_start(args,format);vsnprintf(line,sizeof(line),format,args);va_end(args);
    printf("[PStubePS5] %s\n",line);fflush(stdout);
    if(logFile) {fprintf(logFile,"[PStubePS5] %s\n",line);fflush(logFile);}
}
[[maybe_unused]] uint64_t milliseconds() {
    timespec t{};if(clock_gettime(CLOCK_MONOTONIC,&t)!=0) return 0;
    return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;
}
void openLog() {
    const char* paths[]={"/data/pstube-ps5-probe.log","/mnt/usb0/pstube-ps5-probe.log","/tmp/pstube-ps5-probe.log"};
    for(const char* p:paths) if((logFile=fopen(p,"a"))) {logPath=p;break;}
}
void* loadModule(const char* name) {
    void* result=dlopen(name,RTLD_LAZY);
    const char* error=result?nullptr:dlerror();
    logLine("module=%s loaded=%d error=%s",name,result!=nullptr,error?error:"none");return result;
}
template<class T> bool bind(T& fn,void* module,const char* name) {
    if(!module) return false;
    fn=reinterpret_cast<T>(dlsym(module,name));
    if(!fn) logLine("missing_export=%s",name);
    return fn!=nullptr;
}
struct Notification {char reserved[45];char message[3075];};
void notify(const char* message) {
    using Fn=int(*)(int,Notification*,size_t,int);
    auto fn=reinterpret_cast<Fn>(dlsym(RTLD_DEFAULT,"sceKernelSendNotificationRequest"));
    if(!fn) return;
    Notification n{};snprintf(n.message,sizeof(n.message),"%s",message);
    logLine("notification rc=0x%08x",(unsigned)fn(0,&n,sizeof(n),0));
}
#define API(name) decltype(&name) name=nullptr
double millisecondsPrecise() {timespec t{};if(clock_gettime(CLOCK_MONOTONIC,&t)) return 0;return t.tv_sec*1000.0+t.tv_nsec/1e6;}
struct Video {
    API(sceKernelAllocateMainDirectMemory);API(sceKernelMapDirectMemory);API(sceKernelReleaseDirectMemory);
    API(sceKernelCreateEqueue);API(sceKernelWaitEqueue);API(sceKernelDeleteEqueue);
    API(sceVideoOutOpen);API(sceVideoOutClose);API(sceVideoOutAddFlipEvent);API(sceVideoOutSetFlipRate);
    API(sceVideoOutSubmitFlip);API(sceVideoOutDeleteFlipEvent);API(sceVideoOutSetBufferAttribute2);API(sceVideoOutRegisterBuffers2);
    int handle=-1;intptr_t physical=0;bool allocated=false,eventAdded=false;void* mapped=nullptr;
    struct kevent* events=nullptr;uint32_t* linear=nullptr;PS5_VideoBuf buffers[2]{};
    double tileMs=0,submitMs=0,waitMs=0,lastFlip=0,intervalTotal=0,maxInterval=0;unsigned intervalCount=0,longIntervals=0;
    unsigned width=1920,height=1080;void* ownerModule=nullptr;ParallelTiler tiler;bool parallelTiles=false;
    static constexpr size_t Bytes=0x4000000;
    bool checked(const char* operation,int rc) {logLine("%s rc=0x%08x",operation,(unsigned)rc);return rc==0;}
    bool init(void* module) {
        ownerModule=module;bool ok=true;
#define KERNEL(name) ok=bind(name,RTLD_DEFAULT,#name)&&ok
        KERNEL(sceKernelAllocateMainDirectMemory);KERNEL(sceKernelMapDirectMemory);KERNEL(sceKernelReleaseDirectMemory);
        KERNEL(sceKernelCreateEqueue);KERNEL(sceKernelWaitEqueue);KERNEL(sceKernelDeleteEqueue);
#undef KERNEL
#define VIDEO(name) ok=bind(name,module,#name)&&ok
        VIDEO(sceVideoOutOpen);VIDEO(sceVideoOutClose);VIDEO(sceVideoOutAddFlipEvent);VIDEO(sceVideoOutSetFlipRate);
        VIDEO(sceVideoOutSubmitFlip);VIDEO(sceVideoOutDeleteFlipEvent);VIDEO(sceVideoOutSetBufferAttribute2);VIDEO(sceVideoOutRegisterBuffers2);
#undef VIDEO
        if(!ok) return false;
        handle=sceVideoOutOpen(0xff,0,0,nullptr);logLine("video_open handle=0x%08x",(unsigned)handle);if(handle<0) return false;
        if(!checked("allocate_video_memory",sceKernelAllocateMainDirectMemory(Bytes,0x20000,3,&physical))) return false;allocated=true;
        if(!checked("map_video_memory",sceKernelMapDirectMemory(&mapped,Bytes,0x33,0,physical,0x20000))) return false;
        if(tiledWords(width,height)*4>Bytes/2) return false;
        memset(mapped,0,Bytes);buffers[0].data=mapped;buffers[1].data=static_cast<char*>(mapped)+Bytes/2;
        if(!checked("create_flip_queue",sceKernelCreateEqueue(&events,"pstube platform flip"))) return false;
        if(!checked("add_flip_event",sceVideoOutAddFlipEvent(events,handle,nullptr))) return false;eventAdded=true;
        if(!checked("set_flip_rate",sceVideoOutSetFlipRate(handle,0))) return false;
        PS5_VideoAttr attr{};sceVideoOutSetBufferAttribute2(&attr,0x8000000022000000ULL,0,width,height,0,0,0);
        if(!checked("register_video_buffers",sceVideoOutRegisterBuffers2(handle,0,0,buffers,2,&attr,0,nullptr))) return false;
        linear=static_cast<uint32_t*>(malloc((size_t)width*height*4));
        logLine("linear_frame_allocated=%d video_memory_bytes=%zu",linear!=nullptr,Bytes);
        if(linear && width==3840) parallelTiles=tiler.start();
        logLine("TILING output_width=%u worker_count=%u",width,parallelTiles?2u:1u);
        return linear!=nullptr;
    }
    bool mode(unsigned nextWidth,unsigned nextHeight) {
        if(width==nextWidth && height==nextHeight && linear) return true;
        if(!((nextWidth==1920 && nextHeight==1080) || (nextWidth==3840 && nextHeight==2160))) return false;
        void* module=ownerModule;close();width=nextWidth;height=nextHeight;bool ok=init(module);
        logLine("DISPLAY mode width=%u height=%u ready=%d",width,height,ok);return ok;
    }
    bool frame(unsigned index,uint64_t number) {
        double begun=millisecondsPrecise();
        if(parallelTiles) tiler.frame(linear,static_cast<uint32_t*>(buffers[index].data),width,height);
        else tileFrameFast(linear,static_cast<uint32_t*>(buffers[index].data),width,height);
        double tiled=millisecondsPrecise();tileMs+=tiled-begun;
        int rc=sceVideoOutSubmitFlip(handle,index,1,(int64_t)number);
        if(rc) {logLine("submit_flip rc=0x%08x",(unsigned)rc);return false;}
        double submitted=millisecondsPrecise();submitMs+=submitted-tiled;
        struct kevent event{};int count=0;unsigned timeout=250000;
        rc=sceKernelWaitEqueue(events,&event,1,&count,&timeout);
        if(rc || count!=1) {logLine("wait_flip rc=0x%08x count=%d; stopping display test",(unsigned)rc,count);return false;}
        double flipped=millisecondsPrecise();waitMs+=flipped-submitted;
        if(lastFlip) {double interval=flipped-lastFlip;intervalTotal+=interval;++intervalCount;if(interval>maxInterval) maxInterval=interval;if(interval>25) ++longIntervals;}lastFlip=flipped;
        return true;
    }
    void close() {
        tiler.close();parallelTiles=false;
        if(eventAdded) {checked("remove_flip_event",sceVideoOutDeleteFlipEvent(events,handle));eventAdded=false;}
        if(handle>=0) {sceVideoOutClose(handle);logLine("video_closed");handle=-1;}
        if(events) {checked("delete_flip_queue",sceKernelDeleteEqueue(events));events=nullptr;}
        if(mapped) {checked("unmap_video_memory",munmap(mapped,Bytes));mapped=nullptr;}
        if(allocated) {checked("release_video_memory",sceKernelReleaseDirectMemory(physical,Bytes));allocated=false;}
        free(linear);linear=nullptr;
    }
};
struct Controller {
    API(scePadInit);API(scePadOpen);API(scePadGetHandle);API(scePadReadState);API(scePadClose);
    API(sceUserServiceInitialize);API(sceUserServiceGetLoginUserIdList);
    int handle=-1;bool owned=false,connected=false;
#ifdef PSTUBE_PROTOTYPE
    std::string terminalLine;uint32_t terminalKey=0;bool terminalRelease=false;
#endif
    void init(void* pad,void* users) {
        bool ok=true;
#define PAD(name) ok=bind(name,pad,#name)&&ok
        PAD(scePadInit);PAD(scePadOpen);PAD(scePadGetHandle);PAD(scePadReadState);PAD(scePadClose);
#undef PAD
        ok=bind(sceUserServiceInitialize,users,"sceUserServiceInitialize")&&ok;
        ok=bind(sceUserServiceGetLoginUserIdList,users,"sceUserServiceGetLoginUserIdList")&&ok;
        if(!ok) return;
        int rc=sceUserServiceInitialize(nullptr);logLine("user_service_init rc=0x%08x",(unsigned)rc);
        if(rc && (unsigned)rc!=0x80960003u) return;
        rc=scePadInit();logLine("pad_init rc=0x%08x",(unsigned)rc);
        // Existing userland applications may have initialized the pad already;
        // test the documented handle calls and report their actual results.
        int ids[4]={-1,-1,-1,-1};rc=sceUserServiceGetLoginUserIdList(ids);
        logLine("login_user_list rc=0x%08x",(unsigned)rc);if(rc) return;
        for(int id:ids) {
            if(id<0) continue;
            handle=scePadOpen(id,PS5_PAD_PORT_TYPE_STANDARD,0,nullptr);
            logLine("pad_open handle=0x%08x",(unsigned)handle);
            owned=handle>=0;
            if((unsigned)handle==PS5_PAD_ERROR_ALREADY_OPENED) handle=scePadGetHandle(id,PS5_PAD_PORT_TYPE_STANDARD,0);
            if(handle>=0) return;
        }
    }
    uint32_t read() {
        #ifdef PSTUBE_PROTOTYPE
        // Existing websrv terminal stdin only; no listener or persistent remote service.
        if(terminalRelease) {terminalRelease=false;return 0;}
        pollfd input{STDIN_FILENO,POLLIN,0};
        if(poll(&input,1,0)>0 && (input.revents&POLLIN)) {
            char byte;while(::read(STDIN_FILENO,&byte,1)==1) {
                if(byte=='\n' || byte=='\r') {terminalKey=probe::remoteKey(terminalLine);terminalLine.clear();if(terminalKey) {terminalRelease=true;logLine("UI terminal_button=%u",terminalKey);return terminalKey;}break;}
                if(terminalLine.size()<32) terminalLine+=byte;
                input.revents=0;if(poll(&input,1,0)<=0 || !(input.revents&POLLIN)) break;
            }
        }
        #endif
        if(handle<0) return 0;
        PS5_PadData data{};int rc=scePadReadState(handle,&data);
        if(rc) {logLine("pad_read rc=0x%08x",(unsigned)rc);if(owned) {logLine("pad_close rc=0x%08x",(unsigned)scePadClose(handle));owned=false;}handle=-1;connected=false;return 0;}
        connected=data.connected!=0;return connected?data.buttons:0;
    }
    void close() {if(owned && handle>=0) logLine("pad_close rc=0x%08x",(unsigned)scePadClose(handle));owned=false;handle=-1;}
};
struct Audio {
    API(sceAudioOutInit);API(sceAudioOutOpen);API(sceAudioOutOutput);API(sceAudioOutClose);
    bool ready=false;
    void init(void* module) {
        bool ok=true;
#define AUDIO(name) ok=bind(name,module,#name)&&ok
        AUDIO(sceAudioOutInit);AUDIO(sceAudioOutOpen);AUDIO(sceAudioOutOutput);AUDIO(sceAudioOutClose);
#undef AUDIO
        if(ok) {int rc=sceAudioOutInit();logLine("audio_init rc=0x%08x",(unsigned)rc);ready=rc==0;}
    }
    void tone() {
        if(!ready) {logLine("tone unavailable");return;}
        int h=sceAudioOutOpen(0xff,0,0,1024,48000,1);logLine("audio_open handle=0x%08x",(unsigned)h);if(h<1) return;
        alignas(16) int16_t pcm[2048];unsigned phase=0;int outputs=0;
        for(unsigned block=0;block<12;++block) {
            for(unsigned i=0;i<1024;++i) {int value=(int)(phase%100);value=value<50?value:100-value;int16_t sample=(int16_t)((value-25)*40);pcm[2*i]=pcm[2*i+1]=sample;++phase;}
            int rc=sceAudioOutOutput(h,pcm);if(rc<0) {logLine("audio_output rc=0x%08x",(unsigned)rc);break;}++outputs;
        }
        memset(pcm,0,sizeof(pcm));int rc=sceAudioOutOutput(h,pcm);logLine("audio_silence rc=0x%08x",(unsigned)rc);
        logLine("tone_blocks=%d audio_close rc=0x%08x",outputs,(unsigned)sceAudioOutClose(h));
    }
};
#undef API
bool probeDecoder(void* module) {
    const char* names[]={"sceVideodec2QueryComputeMemoryInfo","sceVideodec2AllocateComputeQueue","sceVideodec2QueryDecoderMemoryInfo","sceVideodec2CreateDecoder","sceVideodec2Decode","sceVideodec2Reset","sceVideodec2Flush","sceVideodec2DeleteDecoder"};
    unsigned found=0;
    for(const char* name:names) {bool exists=module && dlsym(module,name);found+=exists;logLine("decoder_export=%s found=%d",name,exists);}
    logLine("decoder_exports=%u/%zu hardware_decode_tested=0; no decoder allocation or decode calls",found,sizeof(names)/sizeof(names[0]));
    return found==sizeof(names)/sizeof(names[0]);
}
#ifdef PSTUBE_LOCAL_PLAYBACK
#ifdef PSTUBE_AV_PLAYBACK
struct AudioSinkState {Audio* audio;int handle=-1;unsigned blocks=0;};
bool outputPcm(void* p,const int16_t* pcm) {auto& state=*static_cast<AudioSinkState*>(p);int rc=state.audio->sceAudioOutOutput(state.handle,pcm);if(rc<0) {logLine("PCM output rc=0x%08x",(unsigned)rc);return false;}++state.blocks;return true;}
#endif
struct PlaybackDisplay {Video* video;Controller* controller;uint64_t flips=0;bool cancelled=false;bool quit=false,next=false,previousVideo=false,paused=false;uint32_t previous=0;probe::PcmQueue* pcm=nullptr;double controlsUntil=0,positionMs=0;int pictureDim=0,lastDim=-1;
#ifdef PSTUBE_PROTOTYPE
    std::function<bool()> settings{};std::function<void(unsigned)> debug{};std::function<void(bool,bool,unsigned)> controls{};std::function<uint32_t(uint32_t)> controlsInput{};std::function<uint32_t(unsigned)> scriptedInput{};
#endif
};
bool presentLocal(void* opaque,const AVFrame* picture,SwsContext* scaler,unsigned height,int threads,unsigned number,double* conversion,double* presentation) {
    auto& state=*static_cast<PlaybackDisplay*>(opaque);
    uint32_t buttons=state.controller->read(),pressed=buttons&~state.previous;state.previous=buttons;
#ifdef PSTUBE_PROTOTYPE
    if(state.scriptedInput) pressed|=state.scriptedInput(number);
#endif
    if(pressed) state.controlsUntil=probe::benchmarkTime()+3;
    #ifdef PSTUBE_PROTOTYPE
    if(state.controlsInput) pressed=state.controlsInput(pressed);
    if(state.cancelled || state.next || state.previousVideo) return false;
    #endif
    if(pressed&PS5_PAD_BUTTON_OPTIONS) {state.quit=true;state.cancelled=true;return false;}
    if(pressed&PS5_PAD_BUTTON_CIRCLE) {state.cancelled=true;return false;}
    if(pressed&PS5_PAD_BUTTON_R1) {state.next=true;return false;}
    if(pressed&PS5_PAD_BUTTON_L1) {state.previousVideo=true;return false;}
#ifdef PSTUBE_PROTOTYPE
    if(state.pcm && (pressed&PS5_PAD_BUTTON_SQUARE) && state.settings) {state.pcm->pause(true);bool resume=state.settings();state.pcm->pause(false);state.previous=state.controller->read();if(!resume) return false;}
#endif
    #ifdef PSTUBE_PROTOTYPE
    if(pressed&PS5_PAD_BUTTON_TRIANGLE) {if(state.settings && state.pcm) {state.pcm->pause(true);bool resume=state.settings();state.pcm->pause(false);state.previous=state.controller->read();if(!resume) return false;}}
    #endif
    if(state.pcm && (pressed&PS5_PAD_BUTTON_CROSS)) {
        state.paused=true;state.pcm->pause(true);logLine("PLAYER paused=1");
        std::vector<uint32_t> pausedBase((size_t)state.video->width*state.video->height,0);int pauseW=probe::fitWidth(picture->width,picture->height,state.video->width,state.video->height),pauseH=probe::fitHeight(picture->width,picture->height,state.video->width,state.video->height);
        uint8_t* pausePlanes[]={reinterpret_cast<uint8_t*>(pausedBase.data()+((state.video->height-pauseH)/2)*state.video->width+(state.video->width-pauseW)/2),nullptr,nullptr,nullptr};int pauseStrides[]={(int)state.video->width*4,0,0,0};
        if(sws_scale(scaler,picture->data,picture->linesize,0,picture->height,pausePlanes,pauseStrides)!=pauseH) {state.pcm->pause(false);return false;}
        memcpy(state.video->linear,pausedBase.data(),pausedBase.size()*4);
        #ifdef PSTUBE_PROTOTYPE
        if(state.controls) {memcpy(state.video->linear,pausedBase.data(),pausedBase.size()*4);state.controls(true,true,number);if(!state.video->frame(state.flips%2,state.flips)) return false;++state.flips;}
        #endif
        while(state.paused) {usleep(16000);buttons=state.controller->read();pressed=buttons&~state.previous;state.previous=buttons;
            #ifdef PSTUBE_PROTOTYPE
            if(state.controlsInput) pressed=state.controlsInput(pressed);
            if(state.cancelled || state.next || state.previousVideo) state.paused=false;
            if(state.controls) {memcpy(state.video->linear,pausedBase.data(),pausedBase.size()*4);state.controls(true,true,number);if(!state.video->frame(state.flips%2,state.flips)) {state.cancelled=true;state.paused=false;}else ++state.flips;}
            #endif
            if(pressed&PS5_PAD_BUTTON_R1) {state.next=true;state.paused=false;}if(pressed&PS5_PAD_BUTTON_L1) {state.previousVideo=true;state.paused=false;}
            if(pressed&PS5_PAD_BUTTON_CROSS) state.paused=false;
            if(pressed&(PS5_PAD_BUTTON_OPTIONS|PS5_PAD_BUTTON_CIRCLE)) {state.quit=pressed&PS5_PAD_BUTTON_OPTIONS;state.cancelled=true;state.paused=false;}
        }
        state.pcm->pause(false);logLine("PLAYER paused=0");if(state.cancelled || state.next || state.previousVideo) return false;
    }
    double before=probe::benchmarkTime();int outWidth=probe::fitWidth(picture->width,picture->height,state.video->width,state.video->height),outHeight=probe::fitHeight(picture->width,picture->height,state.video->width,state.video->height);
    int x=((int)state.video->width-outWidth)/2,y=((int)state.video->height-outHeight)/2;if(outWidth!=(int)state.video->width || outHeight!=(int)state.video->height) memset(state.video->linear,0,(size_t)state.video->width*state.video->height*4);
    uint8_t* planes[]={reinterpret_cast<uint8_t*>(state.video->linear+y*state.video->width+x),nullptr,nullptr,nullptr};int strides[]={(int)state.video->width*4,0,0,0};
    if(state.lastDim!=state.pictureDim) {
        int* inverse=nullptr;int* table=nullptr;int sourceRange=0,destinationRange=0,brightness=0,contrast=0,saturation=0;
        int dimResult=-1;if(sws_getColorspaceDetails(scaler,&inverse,&sourceRange,&table,&destinationRange,&brightness,&contrast,&saturation)>=0) {int gain=65536*(100-state.pictureDim)/100;dimResult=sws_setColorspaceDetails(scaler,inverse,sourceRange,table,destinationRange,0,gain,gain);}
        logLine("PLAYER picture_dimming percent=%d applied=%d",state.pictureDim,dimResult>=0);if(dimResult<0) state.pictureDim=0;
        state.lastDim=state.pictureDim;
    }
    int rows=sws_scale(scaler,picture->data,picture->linesize,0,picture->height,planes,strides);
    if(rows!=outHeight) {logLine("PLAY conversion_rows=%d",rows);return false;}
    #ifndef PSTUBE_PROTOTYPE
    char label[96];snprintf(label,sizeof(label),"VP NINE %s %s",height==1080?"FULL HD":height==1440?"TWO K":height==2160?"FOUR K":"VIDEO",threads==4?"FOUR THREADS":"SIX THREADS");
    unsigned scale=state.video->width/1920;
    probe::rect(state.video->linear,0,0,state.video->width,52*scale,0xff20120c,state.video->width,state.video->height);probe::text(state.video->linear,20*scale,12*scale,label,3*scale,state.video->width,state.video->height);
    if(state.pcm && (probe::benchmarkTime()<state.controlsUntil)) {
        probe::rect(state.video->linear,0,1020*scale,state.video->width,60*scale,0xff20120c,state.video->width,state.video->height);probe::text(state.video->linear,20*scale,1035*scale,"CROSS PAUSE CIRCLE BACK R ONE NEXT OPTIONS EXIT",3*scale,state.video->width,state.video->height);
    }
    #endif
#ifdef PSTUBE_PROTOTYPE
    if(number==0) logLine("PLAYER controls_start visible=%d input_triggered=%d",probe::benchmarkTime()<state.controlsUntil,pressed!=0);
    if(state.controls) state.controls(probe::benchmarkTime()<state.controlsUntil,false,number);
    if(state.debug) state.debug(number);
#endif
    *conversion=(probe::benchmarkTime()-before)*1000;before=probe::benchmarkTime();
    bool ok=state.video->frame(state.flips%2,state.flips);*presentation=(probe::benchmarkTime()-before)*1000;
    if(ok) ++state.flips;if(number%120==0) logLine("PLAY progress source_height=%u threads=%d displayed=%u",height,threads,number);return ok;
}
#endif

#ifdef PSTUBE_PROTOTYPE
#include "prototype_ui.h"
#endif

}
int main(int argc,char** argv) {
    (void)argc;(void)argv;
    openLog();logLine("PStube PS5 Platform Test 0.20; target=PS5 Pro FW13.00 sdk=v0.45");
    logLine("log=%s pid=%d uid=%d",logPath,getpid(),getuid());
    struct utsname os{};if(uname(&os)==0) logLine("os=%s release=%s machine=%s",os.sysname,os.release,os.machine);
    auto model=reinterpret_cast<int(*)(char*)>(dlsym(RTLD_DEFAULT,"sceKernelGetHwModelName"));
    if(model) {char name[1024]{};int rc=model(name);name[sizeof(name)-1]=0;logLine("model rc=0x%08x name=%s",(unsigned)rc,rc?"unknown":name);}
    char message[256];snprintf(message,sizeof(message),"PStube PS5 Test 0.13 started. Log: %s",logPath);notify(message);
    void* decoder=loadModule("libSceVideodec2.sprx");[[maybe_unused]] bool exports=probeDecoder(decoder);
    void* system=loadModule("libSceSystemService.sprx");
#ifndef PSTUBE_HEADLESS
    auto hide=reinterpret_cast<int(*)(void)>(system?dlsym(system,"sceSystemServiceHideSplashScreen"):nullptr);
    if(hide) logLine("hide_splash rc=0x%08x",(unsigned)hide());
#endif
    void* videoModule=loadModule("libSceVideoOut.sprx");
    void* padModule=loadModule("libScePad.sprx");void* userModule=loadModule("libSceUserService.sprx");
    void* audioModule=loadModule("libSceAudioOut.sprx");
    Controller controller;controller.init(padModule,userModule);Audio audio;audio.init(audioModule);
    Video video;
#ifdef PSTUBE_HEADLESS
    bool display=false;logLine("headless diagnostic: display allocation skipped");
#else
    bool display=video.init(videoModule);logLine("display_ready=%d",display);
#endif
    #ifdef PSTUBE_SOFTWARE_BENCH
    probe::SoftwareBenchmark benchmark(logLine);bool benchStarted=false;uint64_t finishedAt=0,lastDraw=0;
    logLine("software_benchmark auto_start=1 display_during_decode_hz=4 network_audio_scaling_excluded=1");
#endif
#ifdef PSTUBE_LOCAL_PLAYBACK
    [[maybe_unused]] PlaybackDisplay playback{&video,&controller};[[maybe_unused]] unsigned completed=0;
#ifdef PSTUBE_PROTOTYPE
    if(display) runPrototype(video,controller,audio,argc>1 && !strncmp(argv[1],"--smoke",7),argc>1?argv[1]:"");
#else
    if(display) {
#ifdef PSTUBE_AV_PLAYBACK
        probe::networkProbe(logLine);bool network=probe::curlProbe(logLine);logLine("PROTOTYPE network_available=%d",network);
        AudioSinkState sink{&audio};if(audio.ready) sink.handle=audio.sceAudioOutOpen(0xff,0,0,1024,48000,1);
        logLine("PCM open handle=0x%08x",(unsigned)sink.handle);
        if(sink.handle>0) {
            auto* pcm=new probe::PcmQueue(outputPcm,&sink,logLine);
            bool started=pcm->start();bool played=started && probe::playAv("/data/homebrew/PStube-PS5-Test/assets/av-1080.webm",presentLocal,&playback,*pcm,logLine);
            pcm->finish(played);delete pcm;
            logLine("PCM close blocks=%u rc=0x%08x",sink.blocks,(unsigned)audio.sceAudioOutClose(sink.handle));if(played) ++completed;
        }
#else
        const unsigned heights[]={1080,1440,2160};const int threadCounts[]={4,6};bool running=true;
        for(unsigned height:heights) for(int threads:threadCounts) {
            if(!running) break;
            video.tileMs=video.submitMs=video.waitMs=video.lastFlip=video.intervalTotal=video.maxInterval=0;video.intervalCount=video.longIntervals=0;uint64_t beforeFlips=playback.flips;
            running=probe::playLocal(height,threads,presentLocal,&playback,logLine);if(running) ++completed;
            unsigned frames=(unsigned)(playback.flips-beforeFlips);
            logLine("DISPLAY source_height=%u threads=%d frames=%u tile_average_ms=%.3f submit_average_ms=%.3f wait_average_ms=%.3f flip_interval_average_ms=%.3f flip_interval_max_ms=%.3f intervals_over_25ms=%u sync=flip_event",height,threads,frames,frames?video.tileMs/frames:0,frames?video.submitMs/frames:0,frames?video.waitMs/frames:0,video.intervalCount?video.intervalTotal/video.intervalCount:0,video.maxInterval,video.longIntervals);
        }
#endif
    }
    logLine("PLAY series_end completed=%u expected=%u cancelled=%d display_ready=%d flips=%llu",completed,
#ifdef PSTUBE_AV_PLAYBACK
        1u,
#else
        6u,
#endif
        playback.cancelled,display,(unsigned long long)playback.flips);
#endif // PSTUBE_PROTOTYPE
#else
    uint64_t start=milliseconds();uint64_t frame=0,lastLog=0;uint32_t previous=controller.read();
    const char* reason="timeout";bool clockOk=start!=0;
    while(clockOk) {
        uint64_t now=milliseconds();if(!now || now<start) break;
#ifdef PSTUBE_SOFTWARE_BENCH
        if(now-start>=300000) break;
        if(!benchStarted && now-start>=1500) {benchStarted=true;if(!benchmark.start()) {reason="benchmark_start_failed";break;}}
        auto bench=benchmark.snapshot();
        if(bench.done && !finishedAt) finishedAt=now;
        if(finishedAt && now-finishedAt>=10000) {reason="benchmark_complete";break;}
#else
        if(now-start>=15000) break;
#endif
        unsigned elapsed=(unsigned)(now-start);uint32_t buttons=controller.read(),pressed=buttons&~previous;previous=buttons;
        if(pressed) logLine("buttons=0x%08x connected=%d",buttons,controller.connected);
        if(pressed&(PS5_PAD_BUTTON_OPTIONS|PS5_PAD_BUTTON_CIRCLE)) {reason="controller";break;}
        #ifdef PSTUBE_SOFTWARE_BENCH
        if((pressed&PS5_PAD_BUTTON_CROSS) && !benchStarted) {benchStarted=true;if(!benchmark.start()) {reason="benchmark_start_failed";break;}}
#else
        if(pressed&PS5_PAD_BUTTON_CROSS) audio.tone();
#endif
        bool drawNow=true;
#ifdef PSTUBE_SOFTWARE_BENCH
        drawNow=now-lastDraw>=250;
#endif
        if(display && drawNow) {probe::draw(video.linear,elapsed,controller.connected,exports);
#ifdef PSTUBE_SOFTWARE_BENCH
            probe::rect(video.linear,70,470,1780,360,0xff20120c);
            probe::text(video.linear,80,490,"SOFTWARE VP NINE DECODE TEST",4);
            probe::text(video.linear,80,580,bench.line,3);
            probe::text(video.linear,80,680,"OPTIONS OR CIRCLE CANCELS",4);
            probe::text(video.linear,80,770,"RESULTS SAVED TO LOG",4);lastDraw=now;
#endif
            if(!video.frame(frame%2,frame)) {reason="display_error";break;}++frame;}
        else usleep(16000);
        if(now-lastLog>=1000) {logLine("heartbeat elapsed_ms=%u flips=%llu controller=%d audio=%d",elapsed,(unsigned long long)frame,controller.connected,audio.ready);lastLog=now;}
    }
    logLine("test_end reason=%s submitted_completed_flips=%llu",clockOk?reason:"clock_error",(unsigned long long)frame);
#ifdef PSTUBE_SOFTWARE_BENCH
    benchmark.cancelAndJoin();
#endif
    #endif // PSTUBE_LOCAL_PLAYBACK
    video.close();controller.close();
    void* modules[]={audioModule,userModule,padModule,videoModule,system,decoder};
    const char* moduleNames[]={"audio","user","pad","video","system","decoder"};
    for(unsigned i=0;i<6;++i) if(modules[i]) {int rc=dlclose(modules[i]);if(rc) logLine("module_close name=%s rc=%d",moduleNames[i],rc);}
    logLine("cleanup_complete; returning to loader; no hardware decoding performed");
    notify("PStube PS5 Test finished. Retrieve pstube-ps5-probe.log.");
    if(logFile) {fclose(logFile);logFile=nullptr;}
    return 0;
}
