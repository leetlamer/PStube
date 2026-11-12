#pragma once
#include "console_curl.h"
#include "shared/json.h"
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
namespace probe {
struct NetworkPreferences {
    console_network_settings value{};
    std::string message;
    NetworkPreferences() {value.proxy_port=8080;}
    static const char* dnsLabel(int mode) {const char* names[]={"System IPv4 (PS5)","Google DNS over HTTPS","Cloudflare DNS over HTTPS","Custom DNS over HTTPS"};return mode>=0 && mode<=3?names[mode]:"Invalid DNS configuration";}
    static const char* proxyLabel(int mode) {const char* names[]={"Off","HTTP CONNECT","HTTPS CONNECT","SOCKS5 (proxy DNS)"};return mode>=0 && mode<=3?names[mode]:"Invalid proxy configuration";}
    bool valid(std::string& error) const {
        if(value.dns_mode<0 || value.dns_mode>3 || value.proxy_mode<0 || value.proxy_mode>3) {error="Choose a supported DNS and proxy mode";return false;}
        if(value.dns_mode==3 && !value.proxy_mode) {std::string url=value.doh_url;if(url.compare(0,8,"https://") || url.find_first_of("\r\n\t @#\\")!=std::string::npos || url.find('/',8)==std::string::npos || url.find('/',8)==8) {error="Custom DNS needs a valid HTTPS URL without credentials";return false;}}
        if(value.proxy_mode) {std::string host=value.proxy_host;if(host.empty() || host.size()>253 || host.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-")!=std::string::npos || value.proxy_port<1 || value.proxy_port>65535) {error="Proxy needs an IPv4 address/hostname and port1–65535";return false;}}
        if(std::string(value.proxy_user).find_first_of("\r\n")!=std::string::npos || std::string(value.proxy_password).find_first_of("\r\n")!=std::string::npos) {error="Proxy credentials contain invalid characters";return false;}return true;
    }
    void apply() const {console_curl_set_network(&value);}
    bool load(const char* path) {
        FILE* file=fopen(path,"rb");if(!file) {if(errno==ENOENT) return true;value.proxy_mode=-1;message="Network settings unreadable; networking blocked until reset";return false;}char bytes[8193];size_t n=fread(bytes,1,sizeof(bytes),file);bool ok=!ferror(file) && n<sizeof(bytes);fclose(file);
        smarttube::Json json;std::string error;if(!ok || !smarttube::parseJson(std::string(bytes,n),json,error)) {value.proxy_mode=-1;message="Invalid network file; networking blocked until reset";return false;}
        for(const char* key:{"dns","proxy","port"}) if(!json.get(key) || json.get(key)->type!=smarttube::Json::Number) {value.proxy_mode=-1;message="Invalid network settings schema";return false;}
        value.dns_mode=json.integer("dns",0);value.proxy_mode=json.integer("proxy",0);value.proxy_port=json.integer("port",8080);
        auto copy=[&](char* target,size_t size,const char* key) {auto text=json.string(key);if(text.size()>=size) return false;memcpy(target,text.c_str(),text.size()+1);return true;};
        ok=copy(value.doh_url,sizeof(value.doh_url),"doh") && copy(value.proxy_host,sizeof(value.proxy_host),"host") && copy(value.proxy_user,sizeof(value.proxy_user),"username") && copy(value.proxy_password,sizeof(value.proxy_password),"password");
        if(!ok || !valid(error)) {value.proxy_mode=-1;message="Invalid network file; networking blocked until reset";return false;}return true;
    }
    bool save(const char* path) const {
        std::string error;if(!valid(error)) return false;
        std::string bytes="{\"dns\":"+std::to_string(value.dns_mode)+",\"proxy\":"+std::to_string(value.proxy_mode)+",\"port\":"+std::to_string(value.proxy_port)+",\"doh\":"+smarttube::quoteJson(value.doh_url)+",\"host\":"+smarttube::quoteJson(value.proxy_host)+",\"username\":"+smarttube::quoteJson(value.proxy_user)+",\"password\":"+smarttube::quoteJson(value.proxy_password)+"}";
        std::string temporary=std::string(path)+".part";int fd=open(temporary.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0) return false;bool ok=fchmod(fd,0600)==0;size_t at=0;
        while(ok && at<bytes.size()) {auto n=write(fd,bytes.data()+at,bytes.size()-at);if(n<=0) ok=false;else at+=n;}
        if(close(fd)) ok=false;if(!ok || rename(temporary.c_str(),path)) {remove(temporary.c_str());return false;}return true;
    }
};
}
