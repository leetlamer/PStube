#include <initializer_list>
#include "ps5_abi.h"
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
#include <assert.h>
static void* mockDlsym(void*,const char*);
static int mockMunmap(void*,size_t);
#define dlsym mockDlsym
#define munmap mockMunmap
#define main probeMain
#include "../src/main.cpp"
#undef main
#undef munmap
#undef dlsym
namespace {
int failAt=0,step=0,allocatedCount=0,closedCount=0,releasedCount=0,deletedCount=0,unmappedCount=0;
void* memory=nullptr;bool missing=false,padBorrowed=false,padReadFailure=false;int padCloses=0;
int result() {return ++step==failAt?-1:0;}
}
extern "C" {
int sceUserServiceInitialize(void*) {return 0;}
int sceUserServiceGetLoginUserIdList(int ids[4]) {ids[0]=1;return 0;}
int scePadInit() {return 0;}
int scePadOpen(int,int,int,void*) {return padBorrowed?(int)PS5_PAD_ERROR_ALREADY_OPENED:7;}
int scePadGetHandle(int,int,int) {return 7;}
int scePadReadState(int,PS5_PadData* data) {data->connected=1;data->buttons=PS5_PAD_BUTTON_CROSS;return padReadFailure?-1:0;}
int scePadClose(int) {++padCloses;return 0;}
int sceKernelAllocateMainDirectMemory(size_t,size_t,int,intptr_t* out) {int rc=result();if(!rc) {*out=0;allocatedCount++;}return rc;}
int sceKernelMapDirectMemory(void** out,size_t size,int,int,intptr_t,size_t) {int rc=result();if(!rc) {*out=memory=malloc(size);assert(memory);}return rc;}
int sceKernelReleaseDirectMemory(intptr_t,size_t) {assert(!memory);releasedCount++;return 0;}
int sceKernelCreateEqueue(struct kevent** out,const char*) {int rc=result();if(!rc) *out=(struct kevent*)0x1234;return rc;}
int sceKernelWaitEqueue(struct kevent*,struct kevent*,int,int* count,unsigned*) {*count=1;return result();}
int sceKernelDeleteEqueue(struct kevent*) {deletedCount++;return 0;}
int sceVideoOutOpen(int,int,int,const void*) {int rc=result();return rc?-1:7;}
void sceVideoOutClose(int h) {assert(h==7);closedCount++;}
int sceVideoOutAddFlipEvent(struct kevent*,int,void*) {return result();}
int sceVideoOutSetFlipRate(int,int) {return result();}
int sceVideoOutSubmitFlip(int,int,uint32_t,int64_t) {return result();}
int sceVideoOutDeleteFlipEvent(struct kevent*,int) {return 0;}
void sceVideoOutSetBufferAttribute2(PS5_VideoAttr*,uint64_t,uint32_t,uint32_t,uint32_t,uint64_t,uint32_t,uint64_t) {}
int sceVideoOutRegisterBuffers2(int,int,int,PS5_VideoBuf*,int,PS5_VideoAttr*,int,void*) {return result();}
}
static int mockMunmap(void* p,size_t) {assert(p==memory);free(memory);memory=nullptr;unmappedCount++;return 0;}
static void* mockDlsym(void*,const char* name) {
    if(missing) return nullptr;
#define SYMBOL(fn) if(!strcmp(name,#fn)) return reinterpret_cast<void*>(&fn)
    SYMBOL(sceUserServiceInitialize);SYMBOL(sceUserServiceGetLoginUserIdList);SYMBOL(scePadInit);SYMBOL(scePadOpen);SYMBOL(scePadGetHandle);SYMBOL(scePadReadState);SYMBOL(scePadClose);
    SYMBOL(sceKernelAllocateMainDirectMemory);SYMBOL(sceKernelMapDirectMemory);SYMBOL(sceKernelReleaseDirectMemory);
    SYMBOL(sceKernelCreateEqueue);SYMBOL(sceKernelWaitEqueue);SYMBOL(sceKernelDeleteEqueue);
    SYMBOL(sceVideoOutOpen);SYMBOL(sceVideoOutClose);SYMBOL(sceVideoOutAddFlipEvent);SYMBOL(sceVideoOutSetFlipRate);
    SYMBOL(sceVideoOutSubmitFlip);SYMBOL(sceVideoOutDeleteFlipEvent);SYMBOL(sceVideoOutSetBufferAttribute2);SYMBOL(sceVideoOutRegisterBuffers2);
#undef SYMBOL
    return nullptr;
}
int main() {
    // Every staged video initialization failure must release only owned resources.
    for(failAt=1;failAt<=7;++failAt) {
        step=allocatedCount=closedCount=releasedCount=deletedCount=unmappedCount=0;
        Video v;assert(!v.init(reinterpret_cast<void*>(1)));v.close();v.close();
        assert(!memory && releasedCount==allocatedCount && closedCount==(failAt>1?1:0));
        assert(unmappedCount==(failAt>3?1:0));assert(deletedCount==(failAt>4?1:0));
    }
    failAt=0;step=0;allocatedCount=closedCount=releasedCount=deletedCount=unmappedCount=0;
    {Video v;assert(v.init(reinterpret_cast<void*>(1)));probe::draw(v.linear,0,false,false);assert(v.frame(0,0));v.close();}
    assert(allocatedCount==1 && closedCount==1 && releasedCount==1 && unmappedCount==1 && deletedCount==1);
    for(int failure:{8,9}) {failAt=failure;step=0;Video v;assert(v.init(reinterpret_cast<void*>(1)));assert(!v.frame(0,0));v.close();assert(!memory);}
    failAt=0;step=0;
    {Video v;assert(v.init(reinterpret_cast<void*>(1)));assert(v.mode(3840,2160));assert(v.width==3840 && v.height==2160 && tiledWords(v.width,v.height)*4<=Video::Bytes/2);memset(v.linear,0,(size_t)v.width*v.height*4);assert(v.frame(0,1));assert(v.mode(1920,1080));v.close();assert(!memory);}
    failAt=0;step=0;
    {Video v;assert(v.init(reinterpret_cast<void*>(1)));failAt=step+7;assert(!v.mode(3840,2160));failAt=0;assert(v.mode(1920,1080));v.close();assert(!memory);}
    for(bool borrowed:{false,true}) {
        padBorrowed=borrowed;padReadFailure=false;padCloses=0;
        Controller c;c.init(reinterpret_cast<void*>(1),reinterpret_cast<void*>(1));
        assert(c.read()==PS5_PAD_BUTTON_CROSS && c.connected && c.owned==!borrowed);
        padReadFailure=true;assert(!c.read());c.close();c.close();assert(padCloses==(borrowed?0:1));
    }
    puts("PASS: flip submission/wait failures and borrowed-versus-owned controller cleanup");
    missing=true;step=0;{Video v;assert(!v.init(reinterpret_cast<void*>(1)));v.close();}assert(step==0);
    puts("PASS: video init failure cleanup, ownership at physical address zero, successful flip, missing exports and repeat cleanup (host mocks)");
}
