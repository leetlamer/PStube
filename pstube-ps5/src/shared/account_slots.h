#pragma once
#include "oauth.h"
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
namespace smarttube {
// Private local refresh-token slots. Slot0 is always Guest; active selection is
// atomic and persistent. Legacy token path remains a migration input only.
class AccountSlots {
public:
    unsigned active=0;int preferred=-1;std::vector<unsigned> slots;
    void init(const std::string& path,const OAuth& configured) {base_=path;configured_=configured;active=0;preferred=-1;slots.clear();}
    bool restore(OAuth& oauth) {
        const auto path=base_+".accounts.json";FILE* file=fopen(path.c_str(),"rb");
        if(!file) {
            OAuth old=configured_;if(!old.loadTokens(base_.c_str())) return false;
            oauth=old;return remember(oauth,true);
        }
        char bytes[2049];const size_t n=fread(bytes,1,sizeof(bytes),file);const bool ok=!ferror(file) && n<=2048;fclose(file);
        Json json;std::string error;if(!ok || !parseJson(std::string(bytes,n),json,error)) return false;
        if(const auto* entries=json.get("slots")) for(const auto& entry:entries->children) {
            if(entry.type!=Json::Number || entry.text.size()!=1 || entry.text[0]<'1' || entry.text[0]>'5') continue;
            const unsigned slot=(unsigned)(entry.text[0]-'0');if(std::find(slots.begin(),slots.end(),slot)==slots.end()) slots.push_back(slot);
        }
        preferred=json.integer("preferred",-1);if(preferred< -1 || preferred>5 || (preferred>0 && std::find(slots.begin(),slots.end(),(unsigned)preferred)==slots.end())) preferred=-1;
        active=(unsigned)json.integer("active",0);if(preferred>=0) active=(unsigned)preferred;if(active && std::find(slots.begin(),slots.end(),active)==slots.end()) active=0;
        if(!active) {oauth=configured_;oauth.clear();return true;}
        OAuth saved;if(!load(active,saved)) {active=0;oauth=configured_;oauth.clear();persist();return false;}
        oauth=saved;return true;
    }
    bool remember(const OAuth& oauth,bool added=false) {
        if(oauth.refreshToken.empty()) return false;
        unsigned target=added?0:active;
        if(!target) for(unsigned slot:slots) {OAuth saved;if(load(slot,saved) && saved.refreshToken==oauth.refreshToken) {target=slot;break;}}
        if(!target) for(unsigned slot=1;slot<=5;++slot) if(std::find(slots.begin(),slots.end(),slot)==slots.end()) {target=slot;break;}
        if(!target || !oauth.saveTokens(tokenPath(target).c_str())) return false;
        if(std::find(slots.begin(),slots.end(),target)==slots.end()) slots.push_back(target);
        active=target;return persist();
    }
    bool guest(OAuth& oauth) {active=0;oauth=configured_;oauth.clear();return persist();}
    bool cycle(OAuth& oauth) {
        if(oauth.linked() && active && !remember(oauth)) return false;
        const auto current=std::find(slots.begin(),slots.end(),active);
        const unsigned next=!active?(slots.empty()?0:slots.front()):current!=slots.end() && current+1!=slots.end()?*(current+1):0;
        if(!next) return guest(oauth);
        OAuth saved;if(!load(next,saved)) return false;
        active=next;oauth=saved;return persist();
    }
    bool setDefault() {preferred=(int)active;return persist();}
    bool useLastSelected() {preferred=-1;return persist();}
    bool forget(OAuth& oauth) {
        if(preferred==(int)active) preferred=-1;
        if(active && remove(tokenPath(active).c_str())!=0) return false;
        slots.erase(std::remove(slots.begin(),slots.end(),active),slots.end());
        // Remove legacy input after explicit sign-out, so deleted accounts do not migrate again.
        remove(base_.c_str());return guest(oauth);
    }
private:
    std::string base_;OAuth configured_;
    std::string tokenPath(unsigned slot) const {return base_+".slot"+std::to_string(slot);}
    bool load(unsigned slot,OAuth& oauth) const {oauth=configured_;if(oauth.loadTokens(tokenPath(slot).c_str())) return true;oauth=OAuth{};return oauth.loadTokens(tokenPath(slot).c_str());}
    bool persist() const {
        const auto path=base_+".accounts.json",temp=path+".part";
        std::string data="{\"preferred\":"+std::to_string(preferred)+",\"active\":"+std::to_string(active)+",\"slots\":[";for(size_t i=0;i<slots.size();++i) {if(i) data+=",";data+=std::to_string(slots[i]);}data+="]}";
        const int fd=open(temp.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0) return false;
        bool ok=fchmod(fd,0600)==0;size_t at=0;while(ok && at<data.size()) {const auto n=write(fd,data.data()+at,data.size()-at);if(n<=0) ok=false;else at+=(size_t)n;}
        if(close(fd)!=0) ok=false;if(!ok || rename(temp.c_str(),path.c_str())!=0) {remove(temp.c_str());return false;}return true;
    }
};
}
