#pragma once
#include "json.h"
namespace smarttube {
struct AccountProfile {std::string name,photo;};
inline std::string profileText(const Json* node) {
    if(!node) return {};if(node->type==Json::String) return node->text;
    auto text=node->string("simpleText");if(!text.empty()) return text;
    if(const auto* runs=node->get("runs")) for(const auto& run:runs->children) text+=run.string("text");return text;
}
inline bool profilePhotoUrl(const std::string& url) {
    if(url.compare(0,8,"https://") || url.size()>2048 || url.find_first_of("\r\n\t @\\")!=std::string::npos) return false;
    const auto end=url.find_first_of("/?#",8);const auto host=url.substr(8,end==std::string::npos?end:end-8);
    return host=="yt3.ggpht.com" || host=="yt3.googleusercontent.com" || host=="lh3.googleusercontent.com" || host=="lh4.googleusercontent.com";
}
inline AccountProfile accountProfile(const Json& root) {
    AccountProfile out;
    const auto visit=[&](const auto& self,const Json& node)->void {
        for(const char* key:{"accountItem","activeAccountHeaderRenderer"}) if(const auto* item=node.get(key)) {
            if(key==std::string("accountItem")) {const auto* selected=item->get("isSelected");if(!selected || selected->type!=Json::Bool || selected->text!="true") continue;}
            auto name=profileText(item->get("accountName"));if(name.empty() || name.size()>256) continue;
            const auto* photo=item->get("accountPhoto");if(!photo) photo=item->get("avatar");std::string url;
            if(photo) if(const auto* thumbs=photo->get("thumbnails")) for(const auto& thumb:thumbs->children) if(profilePhotoUrl(thumb.string("url"))) url=thumb.string("url");
            out={name,url};return;
        }
        for(const auto& child:node.children) {if(!out.name.empty()) return;self(self,child);}
    };
    visit(visit,root);return out;
}
}
