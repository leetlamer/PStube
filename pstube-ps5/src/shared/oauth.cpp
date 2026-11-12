#include "oauth.h"
#include "tv_oauth.h"
#include <chrono>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
namespace smarttube {
namespace {
bool readJson(const char* path,Json& json,std::string* why=nullptr) {
    FILE* f=fopen(path,"rb");if(!f) {if(why) *why=errno==ENOENT?"Configuration file is missing":"Configuration file could not be opened";return false;}std::string s;char b[1024];size_t n;
    while((n=fread(b,1,sizeof(b),f)) && s.size()+n<=16384) s.append(b,n);
    bool ok=feof(f) && !ferror(f);fclose(f);std::string error;
    if(s.compare(0,3,"\xef\xbb\xbf")==0) s.erase(0,3);
    if(!ok) {if(why) *why="Configuration file is too large or unreadable";return false;}
    if(!parseJson(s,json,error)) {if(why) *why="Configuration file contains invalid JSON";return false;}return true;
}
Request form(const char* endpoint,const std::string& body) {Request r;r.url=std::string("https://oauth2.googleapis.com/")+endpoint;r.body=body;r.userAgent="PStube PS5";r.contentType="application/x-www-form-urlencoded";return r;}
}
std::string oauthError(const Json& response) {
    auto code=response.string("error");if(code.empty()) code=response.string("error_code");
    for(const char* allowed:{"invalid_client","unauthorized_client","invalid_scope","invalid_request","access_denied","invalid_grant","authorization_pending","slow_down","expired_token","rate_limit_exceeded"}) if(code==allowed) return code;
    return code.empty()?"none":"unrecognized_error"; // Never log arbitrary response text.
}
std::string oauthErrorMessage(const Json& response) {
    const auto error=oauthError(response);
    if(error=="invalid_client" || error=="unauthorized_client") return "Google rejected the OAuth client. Check the ID, secret and TV client type.";
    if(error=="invalid_scope") return "Google rejected the requested YouTube scope. Check the OAuth project.";
    if(error=="rate_limit_exceeded") return "Google sign-in request limit reached. Wait before retrying.";
    if(error=="access_denied") return "Sign-in was declined.";
    if(error=="expired_token") return "Sign-in code expired. Press CROSS for a new code.";
    return "Google sign-in failed. Check your TV OAuth client configuration.";
}
std::string formEncode(const std::string& value) {
    static const char* hex="0123456789ABCDEF";std::string out;
    for(unsigned char c:value) {if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='-' || c=='_' || c=='.' || c=='~') out+=(char)c;else {out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;
}
bool OAuth::loadConfig(const char* path) {
    Json j;if(!readJson(path,j,&configStatus)) return false;const Json* c=j.get("installed");if(!c) c=&j;
    if(j.get("web")) {configStatus="Use a TVs and Limited Input devices client, not a Web client";return false;}
    auto id=c->string("client_id"),secret=c->string("client_secret");
    if(id.empty() || secret.empty()) {configStatus="Configuration needs client_id and client_secret";return false;}
    const std::string suffix=".apps.googleusercontent.com";
    if(id.size()>512 || secret.size()>512 || id.size()<=suffix.size() || id.compare(id.size()-suffix.size(),suffix.size(),suffix) || id.find("YOUR_")!=std::string::npos || id.find_first_of(" \r\n\t")!=std::string::npos || secret.find_first_of("\r\n")!=std::string::npos) {configStatus="Replace the example values with your Google TV client credentials";return false;}
    clientId=id;clientSecret=secret;tv=false;configStatus="Google TV OAuth client configured";return true;
}
bool OAuth::loadTokens(const char* path) {
    Json j;if(!readJson(path,j)) return false;
    if(clientId.empty() && j.string("mode")=="tv") {
        const auto id=j.string("client_id"),secret=j.string("tv_client_secret");const std::string suffix=".apps.googleusercontent.com";
        if(id.size()<=suffix.size() || id.size()>512 || id.compare(id.size()-suffix.size(),suffix.size(),suffix) || secret.empty() || secret.size()>512) return false;
        clientId=id;clientSecret=secret;deviceId=j.string("tv_device_id");tvAgent=j.string("tv_agent");if(tvAgent.size()>1024 || tvAgent.find_first_of("\r\n")!=std::string::npos) tvAgent.clear();tv=true;configStatus="YouTube TV client restored";
    }
    if(j.string("client_id")!=clientId || ((j.string("mode")=="tv")!=tv)) return false;
    auto refresh=j.string("refresh_token");if(refresh.empty() || refresh.size()>8192) return false;refreshToken=refresh;status="Account linked";return true;
}
bool OAuth::saveTokens(const char* path) const {
    if(refreshToken.empty()) return false;const auto temp=std::string(path)+".part";
    int fd=open(temp.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0) return false;
    if(fchmod(fd,0600)<0) {close(fd);remove(temp.c_str());return false;}
    auto data="{\"client_id\":"+quoteJson(clientId)+",\"refresh_token\":"+quoteJson(refreshToken);
    if(tv) data+=",\"mode\":\"tv\",\"tv_client_secret\":"+quoteJson(clientSecret)+",\"tv_device_id\":"+quoteJson(deviceId)+",\"tv_agent\":"+quoteJson(tvAgent);
    data+="}";
    size_t done=0;while(done<data.size()) {const auto n=write(fd,data.data()+done,data.size()-done);if(n<=0) break;done+=(size_t)n;}
    bool ok=done==data.size();if(close(fd)<0) ok=false;if(!ok || rename(temp.c_str(),path)<0) {remove(temp.c_str());return false;}return true;
}
void OAuth::clear() {accessToken.clear();refreshToken.clear();deviceCode.clear();userCode.clear();verificationUrl.clear();expiresAt=deviceExpires=nextPoll=0;status="Signed out";}
bool OAuth::useTvClient(const std::string& script,std::string& error) {
    std::string id,secret;if(!tvClientPair(script,id,secret,error)) return false;
    if(!clientId.empty() && clientId!=id) {accessToken.clear();refreshToken.clear();expiresAt=0;}
    clientId=id;clientSecret=secret;tv=true;configStatus="YouTube TV client discovered";
    // This is a nonsecret device label, not an authentication token.
    const uint64_t stamp=(uint64_t)std::chrono::steady_clock::now().time_since_epoch().count();
    char value[64];snprintf(value,sizeof(value),"%08x-%04x-4%03x-8%03x-%012llx",(unsigned)(stamp>>32),(unsigned)(stamp>>16)&0xffff,(unsigned)stamp&0xfff,(unsigned)getpid()&0xfff,(unsigned long long)((stamp^(uintptr_t)this)&UINT64_C(0xffffffffffff)));deviceId=value;return true;
}
Request OAuth::beginRequest() const {
    if(!tv) return form("device/code","client_id="+formEncode(clientId)+"&scope="+formEncode("https://www.googleapis.com/auth/youtube.readonly"));
    Request r;r.url="https://www.youtube.com/o/oauth2/device/code";r.userAgent=tvAgent.empty()?tvUserAgent(0):tvAgent;
    r.body="{\"client_id\":"+quoteJson(clientId)+",\"device_id\":"+quoteJson(deviceId)+",\"model_name\":\"ytlr::\",\"scope\":\"http://gdata.youtube.com https://www.googleapis.com/auth/youtube-paid-content\"}";return r;
}
Request OAuth::pollRequest() const {
    if(!tv) return form("token","client_id="+formEncode(clientId)+"&client_secret="+formEncode(clientSecret)+"&device_code="+formEncode(deviceCode)+"&grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code");
    Request r;r.url="https://www.youtube.com/o/oauth2/token";r.userAgent=beginRequest().userAgent;
    r.body="{\"code\":"+quoteJson(deviceCode)+",\"client_id\":"+quoteJson(clientId)+",\"client_secret\":"+quoteJson(clientSecret)+",\"grant_type\":\"http://oauth.net/grant_type/device/1.0\"}";return r;
}
Request OAuth::refreshRequest() const {
    if(!tv) return form("token","client_id="+formEncode(clientId)+"&client_secret="+formEncode(clientSecret)+"&refresh_token="+formEncode(refreshToken)+"&grant_type=refresh_token");
    Request r;r.url="https://www.youtube.com/o/oauth2/token";r.userAgent=beginRequest().userAgent;
    r.body="{\"refresh_token\":"+quoteJson(refreshToken)+",\"client_id\":"+quoteJson(clientId)+",\"client_secret\":"+quoteJson(clientSecret)+",\"grant_type\":\"refresh_token\"}";return r;
}
bool OAuth::beginResponse(const Json& j,uint64_t now) {
    deviceCode=j.string("device_code");userCode=j.string("user_code");verificationUrl=j.string("verification_url");if(verificationUrl.empty()) verificationUrl=j.string("verification_uri");
    const int expires=j.integer("expires_in"),poll=j.integer("interval",5);
    if(deviceCode.empty() || deviceCode.size()>8192 || userCode.empty() || userCode.size()>64 || verificationUrl.compare(0,8,"https://") || verificationUrl.size()>512 || expires<=0 || expires>3600 || poll<1 || poll>60) {deviceCode.clear();userCode.clear();verificationUrl.clear();status="Could not start sign-in";return false;}
    interval=(unsigned)poll;deviceExpires=now+expires;nextPoll=now+interval;status="Approve this code in your browser";return true;
}
bool OAuth::tokenResponse(const Json& j,uint64_t now) {
    const auto error=j.string("error");
    if(!error.empty()) {
        if(error=="authorization_pending" || error=="slow_down") {if(error=="slow_down") interval+=5;nextPoll=now+interval;return false;}
        deviceCode.clear();userCode.clear();status=error=="access_denied"?"Sign-in declined":error=="expired_token"?"Sign-in code expired":"Sign-in failed; retry or check client configuration";return false;
    }
    const auto access=j.string("access_token"),refresh=j.string("refresh_token");const int seconds=j.integer("expires_in");
    if(access.empty() || access.size()>8192 || seconds<=0 || seconds>86400 || j.string("token_type")!="Bearer" || access.find_first_of("\r\n")!=std::string::npos) {deviceCode.clear();userCode.clear();status="Invalid token response";return false;}
    accessToken=access;if(!refresh.empty() && refresh.size()<=8192) refreshToken=refresh;expiresAt=now+seconds;deviceCode.clear();userCode.clear();status="Signed in";return true;
}
}
