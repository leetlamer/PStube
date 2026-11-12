#include "shared/app_ui.h"
#include <ctime>
namespace probe {struct Document {std::string body,error;long status=0;size_t limit=0;};}
#include "beta_account.h"
#include <cassert>
#include <cstdio>
int main() {
    unsigned polls=0;probe::BetaAccount account([&](const smarttube::Request& request,size_t)->probe::Document {
        if(request.url.find("device/code")!=std::string::npos) return {R"({"device_code":"fixture","user_code":"TEST-CODE","verification_url":"https://www.google.com/device","expires_in":600,"interval":1})","",200};
        if(request.url.find("oauth2/token")!=std::string::npos) {++polls;return {R"({"error":"authorization_pending"})","HTTP 400",400};}
        return {"{}","",200};
    });
    account.oauth.clientId="fixture.apps.googleusercontent.com";account.oauth.clientSecret="fixture";account.oauth.tv=true;account.oauth.refreshToken="fixture-refresh";account.oauth.accessToken="original-account";account.oauth.expiresAt=time(nullptr)+3600;
    // No refresh token: the synthetic prior account remains in memory; beginning must preserve it on cancel.
    account.slots.init("/tmp/pstube-beta-fixture",account.oauth);assert(account.begin());assert(account.pairing && account.oauth.userCode=="TEST-CODE");account.oauth.nextPoll=0;
    assert(!account.poll() && polls==1 && account.pairing);account.cancelPairing();assert(!account.pairing && account.oauth.accessToken=="original-account");
    auto request=account.browse("FEsubscriptions");assert(request.authorization=="Bearer original-account");smarttube::Json document;std::string error;assert(smarttube::parseJson(request.body,document,error));assert(document.string("browseId")=="FEsubscriptions");
    request=account.browse("FEtopics_live","","",false);assert(request.authorization.empty());
    remove("/tmp/pstube-beta-fixture.slot1");remove("/tmp/pstube-beta-fixture.accounts.json");
    puts("PASS: pairing cancellation restores account; pending flow and browse authorization");
}
