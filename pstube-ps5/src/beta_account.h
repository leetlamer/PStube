#pragma once
#include "shared/oauth.h"
#include "shared/tv_oauth.h"
#include "shared/account_slots.h"
#include "shared/account_profile.h"
#include <functional>
#include <set>
namespace probe {
// All state belongs to the UI thread. Only the supplied document fetch runs off-thread.
class BetaAccount {
public:
    using Fetch=std::function<Document(const smarttube::Request&,size_t)>;
    smarttube::OAuth oauth;smarttube::AccountSlots slots;smarttube::AccountProfile profile;
    std::shared_ptr<smarttube::Image> avatar;Fetch fetch;
    explicit BetaAccount(Fetch request):fetch(std::move(request)) {
        oauth.loadConfig("/data/pstube-ps5-oauth.json");slots.init("/data/pstube-ps5-account.json",oauth);slots.restore(oauth);
    }
    Document json(const smarttube::Request& request,smarttube::Json& out) {
        auto response=fetch(request,4*1024*1024);std::string error;
        if(!smarttube::parseJson(response.body,out,error) && response.error.empty()) response.error="Invalid server response";
        return response;
    }
    bool refresh() {
        uint64_t now=time(nullptr);if(oauth.accessToken.empty() || oauth.expiresAt<=now+60) {
            if(oauth.refreshToken.empty()) {oauth.status="Select login to link this console";return false;}
            smarttube::Json result;json(oauth.refreshRequest(),result);
            if(!oauth.tokenResponse(result,now)) return false;slots.remember(oauth);
        }return !oauth.accessToken.empty();
    }
    bool discover() {
        std::set<std::string> tried;unsigned requests=0;std::string error;
        for(unsigned p=0;p<3;++p) {
            smarttube::Request request;request.httpGet=true;request.url="https://www.youtube.com/tv";request.userAgent=smarttube::tvUserAgent(p);
            auto bootstrap=fetch(request,4*1024*1024);if(!bootstrap.error.empty()) continue;
            if(oauth.useTvClient(bootstrap.body,error)) {oauth.tvAgent=request.userAgent;return true;}
            auto queue=smarttube::tvScriptCandidates(bootstrap.body);
            for(size_t i=0;i<queue.size() && requests<8;++i) {
                if(!smarttube::trustedTvScript(queue[i]) || !tried.insert(std::to_string(p)+queue[i]).second) continue;
                ++requests;request.url=queue[i];auto script=fetch(request,16*1024*1024);if(!script.error.empty()) continue;
                if(oauth.useTvClient(script.body,error)) {oauth.tvAgent=request.userAgent;return true;}
                auto legacy=smarttube::legacyTvScript(queue[i]);if(legacy!=queue[i] && queue.size()<8) queue.push_back(legacy);
                for(auto& child:smarttube::tvScriptCandidates(script.body)) if(queue.size()<8 && std::find(queue.begin(),queue.end(),child)==queue.end()) queue.push_back(child);
            }
        }oauth.status="YouTube TV login configuration unavailable. Retry or configure pstube-ps5-oauth.json.";return false;
    }
    bool begin() {
        if(slots.slots.size()>=5) {oauth.status="Five accounts saved; remove an account before adding another";return false;}
        if(oauth.linked() && !slots.remember(oauth)) {oauth.status="Could not save current account";return false;}
        // Keep the active account intact until pairing succeeds or the user explicitly switches.
        smarttube::OAuth previous=oauth;
        if(!oauth.configured() && !discover()) return false;
        oauth.clear();smarttube::Json result;auto response=json(oauth.beginRequest(),result);
        if(!response.error.empty() || !oauth.beginResponse(result,time(nullptr))) {
            std::string message=response.error.empty()?smarttube::oauthErrorMessage(result):response.error;oauth=previous;oauth.status=message;return false;
        }pendingPrevious=previous;pairing=true;profile={};avatar.reset();return true;
    }
    void cancelPairing() {if(pairing) {oauth=pendingPrevious;pairing=false;profile={};avatar.reset();}}
    bool poll() {
        if(!pairing) return false;uint64_t now=time(nullptr);
        if(now>=oauth.deviceExpires) {cancelPairing();oauth.status="Sign-in code expired";return false;}
        if(now<oauth.nextPoll) return false;
        smarttube::Json result;auto response=json(oauth.pollRequest(),result);
        if(result.type!=smarttube::Json::Object) {oauth.nextPoll=now+oauth.interval;return false;}
        if(oauth.tokenResponse(result,now)) {pairing=false;bool saved=slots.remember(oauth,true);loadProfile();if(!saved) oauth.status="Signed in for this session; token save failed";return true;}
        if(oauth.deviceCode.empty()) {auto message=oauth.status;cancelPairing();oauth.status=message;}return false;
    }
    void loadProfile() {
        profile={};avatar.reset();if(!refresh()) return;
        smarttube::Request request;request.url="https://www.youtube.com/youtubei/v1/account/accounts_list";request.authorization="Bearer "+oauth.accessToken;
        request.userAgent=oauth.tvAgent.empty()?smarttube::tvUserAgent(0):oauth.tvAgent;request.body=R"({"context":{"client":{"clientName":"TVHTML5","clientVersion":"7.20260901.15.00","tvAppInfo":{"appQuality":"TV_APP_QUALITY_FULL_ANIMATION","zylonLeftNav":true},"webpSupport":false,"hl":"en","gl":"US"}}})";
        smarttube::Json result;auto response=json(request,result);if(response.error.empty()) profile=smarttube::accountProfile(result);
        if(!profile.photo.empty()) {request.httpGet=true;request.authorization.clear();request.url=profile.photo;auto image=fetch(request,512*1024);auto decoded=smarttube::decodeThumbnail(image.body);if(decoded) {
            avatar=std::make_shared<smarttube::Image>();avatar->width=avatar->height=360;avatar->pixels.resize(360*360);
            for(int y=0;y<360;++y) std::copy_n(decoded->pixels.data()+y*640+140,360,avatar->pixels.data()+y*360);
        }}
    }
    smarttube::Request browse(const std::string& id,const std::string& continuation="",const std::string& params="",bool authorized=true) {
        smarttube::Request request;request.url="https://www.youtube.com/youtubei/v1/browse";request.clientNumber="7";request.clientVersion="7.20260901.15.00";
        request.userAgent=oauth.tvAgent.empty()?smarttube::tvUserAgent(0):oauth.tvAgent;
        if(authorized && refresh()) request.authorization="Bearer "+oauth.accessToken;
        request.body=R"({"context":{"client":{"clientName":"TVHTML5","clientVersion":"7.20260901.15.00","tvAppInfo":{"appQuality":"TV_APP_QUALITY_FULL_ANIMATION","zylonLeftNav":true},"webpSupport":false,"hl":"en","gl":"US"}},)"+(continuation.empty()?"\"browseId\":"+smarttube::quoteJson(id):"\"continuation\":"+smarttube::quoteJson(continuation))+(params.empty()?"":",\"params\":"+smarttube::quoteJson(params))+"}";return request;
    }
    bool pairing=false;
private:smarttube::OAuth pendingPrevious;
};
}
