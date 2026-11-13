// Exercise the real CurlStream ring/worker with deterministic transport faults.
#include <curl/curl.h>
#include <cstdarg>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <cassert>
#include <cstdio>
#include "console_curl.h"
#undef curl_easy_setopt
#undef curl_easy_getinfo
struct Transfer {curl_write_callback header=nullptr,write=nullptr;void* headers=nullptr;void* body=nullptr;std::string range;long status=0;};
static std::atomic<bool> releaseRefill{false};static std::string payload;static std::atomic<unsigned> calls{0};static int scenario=0;static constexpr size_t split=3*1024*1024+37;
extern "C" CURL* curl_easy_init() {return reinterpret_cast<CURL*>(new Transfer);}
extern "C" void curl_easy_cleanup(CURL* c) {delete reinterpret_cast<Transfer*>(c);}
extern "C" void curl_easy_reset(CURL* c) {*reinterpret_cast<Transfer*>(c)=Transfer{};}
static CURLcode test_setopt(CURL* c,int option,...) {auto& t=*reinterpret_cast<Transfer*>(c);va_list a;va_start(a,option);switch(option) {
case CURLOPT_HEADERFUNCTION:t.header=va_arg(a,curl_write_callback);break;
case CURLOPT_WRITEFUNCTION:t.write=va_arg(a,curl_write_callback);break;
case CURLOPT_HEADERDATA:t.headers=va_arg(a,void*);break;
case CURLOPT_WRITEDATA:t.body=va_arg(a,void*);break;
case CURLOPT_RANGE:t.range=va_arg(a,const char*);break;
default:break;}va_end(a);return CURLE_OK;}
static CURLcode test_getinfo(CURL* c,int info,...) {va_list a;va_start(a,info);if(info==CURLINFO_RESPONSE_CODE) *va_arg(a,long*)=reinterpret_cast<Transfer*>(c)->status;va_end(a);return CURLE_OK;}
extern "C" CURLcode curl_easy_perform(CURL* c) {
    auto& t=*reinterpret_cast<Transfer*>(c);unsigned attempt=calls++;size_t offset=t.range.empty()?0:std::stoull(t.range);assert(offset<=payload.size());
    if(attempt) assert(offset==(scenario==4?32768:split));
    t.status=offset && scenario!=1?206:200;
    auto header=[&](std::string s) {assert(t.header(s.data(),1,s.size(),t.headers)==s.size());};
    header("HTTP/1.1 "+std::to_string(t.status)+" OK\r\n");
    if(t.status==206) header("Content-Range: bytes "+std::to_string(offset)+"-"+std::to_string(payload.size()-1)+"/"+std::to_string(payload.size())+"\r\n");
    else header("Content-Length: "+std::to_string(payload.size())+"\r\n");header("\r\n");
    size_t end=attempt==0 && scenario<3?split:payload.size();
    if(scenario==5) {assert(t.write(payload.data(),1,64000,t.body)==64000);while(!releaseRefill) std::this_thread::yield();offset=64000;}
    for(size_t p=offset;p<end;) {size_t n=std::min((size_t)17003,end-p);if(t.write(payload.data()+p,1,n,t.body)!=n) return CURLE_WRITE_ERROR;p+=n;}
    return attempt==0 && scenario<2?CURLE_RECV_ERROR:CURLE_OK;
}
extern "C" void console_curl_setup(CURL*) {}
extern "C" CURLcode console_curl_apply_network(CURL*,const console_network_settings*) {return CURLE_OK;}
extern "C" const char* console_curl_ca_file() {return "fixture";}
#define curl_easy_setopt test_setopt
#define curl_easy_getinfo test_getinfo
#include "curl_transport.h"
static void logMessage(const char*,...) {}
int main() {
    payload.resize(10*1024*1024+137);for(size_t i=0;i<payload.size();++i) payload[i]=(char)((i*31+i/257)%251);
    for(scenario=0;scenario<3;++scenario) {calls=0;probe::CurlStream stream(logMessage);assert(stream.start("https://fixture.invalid/video","PStube test"));std::string output;uint8_t bytes[12001];int n=0;while((n=probe::CurlStream::read(&stream,bytes,sizeof(bytes)))>0) output.append((char*)bytes,n);stream.close();assert(calls==2);
        if(scenario==1) {assert(n==AVERROR(EIO));assert(output==payload.substr(0,split));}else {assert(n==AVERROR_EOF);assert(output==payload);}}
    scenario=4;calls=0;{probe::CurlStream stream(logMessage,32768);assert(stream.start("https://fixture.invalid/audio","test"));std::string output;uint8_t bytes[3001];int n;while((n=probe::CurlStream::read(&stream,bytes,sizeof(bytes)))>0) output.append((char*)bytes,n);assert(n==AVERROR_EOF && output==payload && calls==2);}
    scenario=3;calls=0;auto start=std::chrono::steady_clock::now();{probe::CurlStream stream(logMessage);assert(stream.start("https://fixture.invalid/video","test"));std::this_thread::sleep_for(std::chrono::milliseconds(40));stream.cancel();stream.close();}assert(std::chrono::steady_clock::now()-start<std::chrono::seconds(1));
    scenario=5;calls=0;releaseRefill=false;{std::atomic<bool> waiting{false};std::atomic<unsigned> ticks{0};probe::CurlStream stream(logMessage);stream.waitTick([&]{++ticks;(void)stream.ready();return true;});stream.configure(128000,false,false,[&](bool value){waiting=value;});assert(stream.start("https://fixture.invalid/video","test"));while(!stream.ready()) std::this_thread::yield();uint8_t bytes[64000];assert(probe::CurlStream::read(&stream,bytes,64000)==64000);assert(!memcmp(bytes,payload.data(),64000));int next=0;std::thread reader([&]{next=probe::CurlStream::read(&stream,bytes,1);});auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!waiting && std::chrono::steady_clock::now()<deadline) std::this_thread::yield();assert(waiting);while(ticks<2 && std::chrono::steady_clock::now()<deadline) std::this_thread::yield();assert(ticks>=2);releaseRefill=true;reader.join();assert(next==1 && bytes[0]==(uint8_t)payload[64000] && !waiting);stream.cancel();stream.close();}
    puts("PASS: exact-byte recovery after receive error/short EOF, strict Range validation, fault diagnostic and full-ring cancellation");
}
