#include "oauth.h"
#include <assert.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fstream>
using namespace smarttube;
Json json(const char* s) {Json j;std::string error;assert(parseJson(s,j,error));return j;}
int main() {
    OAuth auth;assert(!auth.configured());
    const char* config="/tmp/smarttube-oauth-config-test.json";remove(config);
    assert(!auth.loadConfig(config) && auth.configStatus.find("missing")!=std::string::npos);
    {std::ofstream out(config);out<<"not json";}assert(!auth.loadConfig(config) && auth.configStatus.find("JSON")!=std::string::npos);
    {std::ofstream out(config);out<<R"({"client_id":"YOUR_TV_OAUTH_CLIENT_ID.apps.googleusercontent.com","client_secret":"example"})";}assert(!auth.loadConfig(config));
    {std::ofstream out(config);out<<R"({"client_id":"real.apps.googleusercontent.com"})";}assert(!auth.loadConfig(config) && auth.configStatus.find("client_secret")!=std::string::npos);
    {std::ofstream out(config);out<<"\xef\xbb\xbf"<<R"({"installed":{"client_id":"test.apps.googleusercontent.com","client_secret":"fake-secret"}})";}assert(auth.loadConfig(config));remove(config);
    assert(oauthError(json(R"({"error":"invalid_client","error_description":"private-secret"})"))=="invalid_client");
    assert(oauthErrorMessage(json(R"({"error":"invalid_client"})")).find("TV client type")!=std::string::npos);
    assert(oauthError(json(R"({"error":"sensitive-untrusted-response"})"))=="unrecognized_error");auth.clientId="test.apps.googleusercontent.com";auth.clientSecret="test & secret";
    auto begin=auth.beginRequest();assert(begin.url=="https://oauth2.googleapis.com/device/code" && begin.contentType=="application/x-www-form-urlencoded" && begin.body.find("youtube.readonly")!=std::string::npos);
    assert(auth.beginResponse(json(R"({"device_code":"private","user_code":"ABCD-EFGH","verification_url":"https://www.google.com/device","expires_in":600,"interval":5})"),100));
    assert(auth.nextPoll==105 && auth.deviceExpires==700);assert(auth.pollRequest().body.find("test%20%26%20secret")!=std::string::npos);
    assert(!auth.tokenResponse(json(R"({"error":"authorization_pending"})"),105) && auth.nextPoll==110);
    assert(!auth.tokenResponse(json(R"({"error":"slow_down"})"),110) && auth.interval==10 && auth.nextPoll==120);
    assert(auth.tokenResponse(json(R"({"access_token":"access","refresh_token":"refresh","token_type":"Bearer","expires_in":3600})"),120));
    assert(auth.linked() && auth.deviceCode.empty() && auth.expiresAt==3720);
    const char* path="/tmp/smarttube-oauth-test.json";assert(auth.saveTokens(path));struct stat st;assert(!stat(path,&st) && (st.st_mode&0777)==0600);
    OAuth restored;restored.clientId=auth.clientId;assert(restored.loadTokens(path) && restored.refreshToken=="refresh" && restored.accessToken.empty());
    assert(auth.tokenResponse(json(R"({"access_token":"new","token_type":"Bearer","expires_in":3600})"),300));assert(auth.refreshToken=="refresh");
    restored.clientId="wrong-client";assert(!restored.loadTokens(path));remove(path);
    assert(!auth.tokenResponse(json(R"({"access_token":"bad\r\nheader","token_type":"Bearer","expires_in":3600})"),400));
    assert(auth.deviceCode.empty());
    auth.clear();assert(!auth.linked() && auth.refreshToken.empty() && auth.userCode.empty());
    assert(auth.beginResponse(json(R"({"device_code":"x","user_code":"CODE","verification_uri":"https://www.google.com/device","expires_in":60})"),500));
    assert(!auth.tokenResponse(json(R"({"error":"access_denied"})"),510) && auth.deviceCode.empty());
    assert(!auth.beginResponse(json(R"({"device_code":"x","user_code":"CODE","verification_url":"http://insecure","expires_in":60})"),600));
    puts("PASS: OAuth device flow, pending/slow-down/denial, refresh, private token persistence and header rejection");
}
