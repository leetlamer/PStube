#pragma once
// Function signatures from SDK v0.45 samples/http2_get/main.c.
#include <dlfcn.h>
#include "software_benchmark.h"
namespace probe {
inline bool networkProbe(BenchLog log) {
    void* net=dlopen("libSceNet.sprx",RTLD_LAZY);void* ssl=dlopen("libSceSsl.sprx",RTLD_LAZY);void* http=dlopen("libSceHttp2.sprx",RTLD_LAZY);
#define FN(name,type,module) auto name=reinterpret_cast<type>(module?dlsym(module,#name):nullptr)
    FN(sceNetInit,int(*)(),net);FN(sceNetPoolCreate,int(*)(const char*,int,int),net);FN(sceNetPoolDestroy,int(*)(int),net);
    FN(sceSslInit,int(*)(size_t),ssl);FN(sceSslTerm,int(*)(int),ssl);
    FN(sceHttp2Init,int(*)(int,int,size_t,int),http);FN(sceHttp2Term,int(*)(int),http);
    FN(sceHttp2CreateTemplate,int(*)(int,const char*,int,int),http);FN(sceHttp2DeleteTemplate,int(*)(int),http);
    FN(sceHttp2CreateRequestWithURL,int(*)(int,const char*,const char*,uint64_t),http);FN(sceHttp2DeleteRequest,int(*)(int),http);
    FN(sceHttp2SendRequest,int(*)(int,const void*,size_t),http);FN(sceHttp2GetStatusCode,int(*)(int,int*),http);
#undef FN
    bool ok=sceNetInit && sceNetPoolCreate && sceNetPoolDestroy && sceSslInit && sceSslTerm && sceHttp2Init && sceHttp2Term && sceHttp2CreateTemplate && sceHttp2DeleteTemplate && sceHttp2CreateRequestWithURL && sceHttp2DeleteRequest && sceHttp2SendRequest && sceHttp2GetStatusCode;
    int pool=-1,tls=-1,context=-1,tmpl=-1,request=-1,status=0,rc=-1;
    if(ok) {rc=sceNetInit();log("NET init rc=0x%08x",(unsigned)rc);pool=sceNetPoolCreate("pstube ps5",256*1024,0);ok=pool>=0;}
    if(ok) {tls=sceSslInit(1024*1024);ok=tls>=0;}
    if(ok) {context=sceHttp2Init(pool,tls,1024*1024,1);ok=context>=0;}
    if(ok) {tmpl=sceHttp2CreateTemplate(context,"PStube PS5 prototype",3,1);ok=tmpl>=0;}
    if(ok) {request=sceHttp2CreateRequestWithURL(tmpl,"GET","https://www.youtube.com/generate_204",0);ok=request>=0;}
    if(ok) {rc=sceHttp2SendRequest(request,nullptr,0);ok=rc==0;}
    if(ok) {rc=sceHttp2GetStatusCode(request,&status);ok=rc==0 && status==204;}
    log("NET result ok=%d http_status=%d rc=0x%08x pool=%d tls=%d context=%d template=%d request=%d certificate_options_unchanged=1",ok,status,(unsigned)rc,pool,tls,context,tmpl,request);
    if(request>=0) sceHttp2DeleteRequest(request);if(tmpl>=0) sceHttp2DeleteTemplate(tmpl);if(context>=0) sceHttp2Term(context);if(tls>=0) sceSslTerm(tls);if(pool>=0) sceNetPoolDestroy(pool);
    // Same loader-owned module behavior as platform; report failure without force unload.
    if(http) dlclose(http);if(ssl) dlclose(ssl);if(net) dlclose(net);return ok;
}
}
