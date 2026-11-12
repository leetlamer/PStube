#include "tv_oauth.h"
#include "oauth.h"
#include <assert.h>
#include <stdio.h>
using namespace smarttube;
int main() {
    std::string url,error,id,secret;
    assert(tvScriptUrl(R"(<script src="/s/_/kabuki/k=test/dg=0/m=base" id="base-js"></script>)",url,error));
    assert(url=="https://www.youtube.com/s/_/kabuki/k=test/dg=0/m=base");
    assert(legacyTvScript(url)=="https://www.youtube.com/s/_/kabuki/k=test/exm=base/ed=1/m=main");
    assert(tvScriptUrl(R"(x.src = '\/s\/_\/kabuki\/k=test\/m=base'; x.id='base-js')",url,error));
    assert(trustedTvScript(url));
    assert(!tvScriptUrl(R"(<script id="base-js" src="https://attacker.invalid/s/m=base"></script>)",url,error));
    assert(!trustedTvScript("https://www.youtube.com.attacker.invalid/s/m=base"));
    const std::string script=R"(old="wrong.apps.googleusercontent.com";var ignored="x";a={clientId:"first.apps.googleusercontent.com",$b:"PUBLIC_APP_VALUE"};z={clientId:"second.apps.googleusercontent.com",s:"SECOND_VALUE"};)";
    assert(tvClientPair(script,id,secret,error));assert(id=="first.apps.googleusercontent.com" && secret=="PUBLIC_APP_VALUE");
    assert(!tvClientPair(R"(clientId:"first.apps.googleusercontent.com",a:34;other:"PRIVATE")",id,secret,error));
    assert(tvScriptUrl(R"(<script src="/s/tv/loader.js"></script>)",url,error));
    assert(trustedTvScript(url) && tvBootstrapKind("<html>browser unsupported</html>")=="unsupported_client_page");
    const auto children=tvScriptCandidates(R"(loader.src='/s/_/kabuki/module/m=base';extra='/s/_/kabuki/module/m=main')");assert(children.size()==2);
    assert(tvClientPair(R"({"client_id":"json.apps.googleusercontent.com","client_secret":"PUBLIC_VALUE"})",id,secret,error) && id=="json.apps.googleusercontent.com");
    OAuth oauth;assert(oauth.useTvClient(script,error) && oauth.tv && oauth.deviceId.size()==36);
    Json j;assert(parseJson(oauth.beginRequest().body,j,error));assert(j.string("client_id")==oauth.clientId && !j.string("device_id").empty());
    assert(oauth.beginRequest().url=="https://www.youtube.com/o/oauth2/device/code");
    oauth.deviceCode="test-device-code";assert(parseJson(oauth.pollRequest().body,j,error));
    assert(j.string("code")=="test-device-code" && j.string("grant_type")=="http://oauth.net/grant_type/device/1.0");
    assert(oauth.pollRequest().url=="https://www.youtube.com/o/oauth2/token");
    oauth.refreshToken="fake-local-refresh";assert(oauth.saveTokens("/tmp/smarttube-tv-test.json"));OAuth restored;assert(restored.loadTokens("/tmp/smarttube-tv-test.json") && restored.tv && restored.clientId==oauth.clientId && restored.refreshToken==oauth.refreshToken);remove("/tmp/smarttube-tv-test.json");
    puts("PASS: TV script discovery, modern first-pair extraction, host restriction, TV grant and account restoration (synthetic fixtures)");
}
