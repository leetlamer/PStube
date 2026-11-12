#pragma once
#include "core.h"
namespace smarttube {
class OAuth {
public:
    std::string clientId,clientSecret,accessToken,refreshToken;
    bool tv=false;std::string deviceId,tvAgent;
    bool useTvClient(const std::string& script,std::string& error);
    std::string configStatus="Google TV OAuth client is not configured";
    std::string deviceCode,userCode,verificationUrl,status="Not signed in";
    uint64_t expiresAt=0,deviceExpires=0,nextPoll=0;unsigned interval=5;
    bool configured() const {return !clientId.empty();}
    bool linked() const {return !refreshToken.empty() || !accessToken.empty();}
    bool loadConfig(const char* path);
    bool loadTokens(const char* path);
    bool saveTokens(const char* path) const;
    void clear();
    Request beginRequest() const;
    Request pollRequest() const;
    Request refreshRequest() const;
    bool beginResponse(const Json& json,uint64_t now);
    bool tokenResponse(const Json& json,uint64_t now);
};
std::string oauthError(const Json& response);
std::string oauthErrorMessage(const Json& response);
std::string formEncode(const std::string& value);
}
