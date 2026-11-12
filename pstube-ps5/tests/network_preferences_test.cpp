#include "network_preferences.h"
#include <cassert>
#include <cstdio>
static console_network_settings applied{};
extern "C" void console_curl_set_network(const console_network_settings* value) {applied=*value;}
int main() {
    std::string error;probe::NetworkPreferences preferences;assert(preferences.valid(error));preferences.value.dns_mode=3;assert(!preferences.valid(error));
    strcpy(preferences.value.doh_url,"https:///dns-query");assert(!preferences.valid(error));strcpy(preferences.value.doh_url,"https://resolver.example/dns-query");assert(preferences.valid(error));
    preferences.value.proxy_mode=3;strcpy(preferences.value.proxy_host,"proxy.example");preferences.value.proxy_port=1080;strcpy(preferences.value.proxy_user,"fixture-user");strcpy(preferences.value.proxy_password,"fixture-password");assert(preferences.valid(error));
    char path[128];snprintf(path,sizeof(path),"/tmp/pstube-network-fixture-%d.json",getpid());assert(preferences.save(path));struct stat facts;assert(!stat(path,&facts) && (facts.st_mode&0777)==0600);
    probe::NetworkPreferences restored;assert(restored.load(path));assert(restored.value.proxy_mode==3 && !strcmp(restored.value.proxy_password,"fixture-password"));restored.apply();assert(applied.proxy_mode==3);
    strcpy(preferences.value.proxy_host,"user@host");assert(!preferences.valid(error));FILE* file=fopen(path,"wb");assert(file);fputs("{bad json",file);fclose(file);assert(!restored.load(path) && restored.value.proxy_mode==-1);restored.apply();assert(applied.proxy_mode==-1);remove(path);
    puts("PASS: app-only DNS/proxy validation, private persistence, credentials roundtrip and invalid-config fail-closed behavior");
}
