#pragma once
#include "shared/app_ui.h"
#include "network_preferences.h"
namespace probe {
inline void drawNetworkSettings(smarttube::Canvas& canvas,const NetworkPreferences& draft,int row,const std::string& note) {
canvas.rect(0,0,1920,1080,0xff081321);canvas.text("Network settings",270,42,39,0xffeef6ff,1390);
    std::string rows[]={std::string("DNS: ")+probe::NetworkPreferences::dnsLabel(draft.value.dns_mode),std::string("Custom DoH URL: ")+draft.value.doh_url,std::string("Proxy: ")+probe::NetworkPreferences::proxyLabel(draft.value.proxy_mode),std::string("Proxy host: ")+draft.value.proxy_host,"Proxy port: "+std::to_string(draft.value.proxy_port),std::string("Proxy username: ")+draft.value.proxy_user,std::string("Proxy password: ")+(draft.value.proxy_password[0]?"********":"Not set"),"Test connection (without saving)","Save and apply","Back without saving","Reset to system / no proxy"};
    for(int i=0;i<11;++i) {int y=135+i*71;canvas.rect(260,y,1400,59,row==i?0xff329dff:0xff173550);canvas.text(rows[i],282,y+13,25,0xffeef6ff,1350);}
    canvas.text(note,270,937,23,0xff9db4cc,1390,2);canvas.text("LEFT / RIGHT  Change mode   CROSS  Edit / select   CIRCLE  Cancel",270,1020,22,0xff9db4cc,1390);
}
}
