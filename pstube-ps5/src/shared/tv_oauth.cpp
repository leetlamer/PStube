#include "tv_oauth.h"
#include <ctype.h>
namespace smarttube {
namespace {
bool word(unsigned char c) {return isalnum(c) || c=='_' || c=='$';}
void spaces(const std::string& s,size_t& p) {while(p<s.size() && isspace((unsigned char)s[p])) ++p;}
bool quoted(const std::string& s,size_t& p,std::string& out) {
    out.clear();if(p>=s.size() || (s[p]!='\'' && s[p]!='"')) return false;const char quote=s[p++];
    while(p<s.size() && out.size()<8192) {
        char c=s[p++];if(c==quote) return true;
        if(c=='\\') {
            if(p>=s.size()) return false;c=s[p++];
            if(c=='u' || c=='x') {
                const size_t n=c=='u'?4:2;if(n>s.size()-p) return false;unsigned value=0;
                for(size_t i=0;i<n;++i) {const unsigned char digit=s[p++];if(!isxdigit(digit)) return false;value=value*16+(isdigit(digit)?digit-'0':tolower(digit)-'a'+10);}
                if(!value || value>127) return false;c=(char)value;
            } else if(c!='/' && c!='\\' && c!='\'' && c!='"') return false;
        }
        if((unsigned char)c<32) return false;out+=c;
    }
    return false;
}
std::string normalize(std::string value) {
    for(size_t p=0;(p=value.find("&amp;",p))!=std::string::npos;) value.replace(p,5,"&");
    if(value.compare(0,2,"//")==0) value="https:"+value;
    else if(!value.empty() && value[0]=='/') value="https://www.youtube.com"+value;
    return value;
}
bool attribute(const std::string& tag,const std::string& key,std::string& value) {
    size_t at=0;while((at=tag.find(key,at))!=std::string::npos) {
        const auto begin=at;at+=key.size();if(begin && word((unsigned char)tag[begin-1])) continue;
        size_t p=at;spaces(tag,p);if(p>=tag.size() || tag[p++]!='=') continue;spaces(tag,p);if(quoted(tag,p,value)) return true;
    }return false;
}
}
const char* tvUserAgent(unsigned index) {
    static const char* profiles[]={
        "Mozilla/5.0 (Linux armeabi-v7a; Android 7.1.2; Fire OS 6.0) Cobalt/22.lts.3.306369-gold (unlike Gecko) v8/8.8.278.8-jit gles Starboard/13, Amazon_ATV_mediatek8695_2019/NS6294 (Amazon, AFTMM, Wireless) com.amazon.firetv.youtube/22.3.r2.v66.0",
        "Mozilla/5.0 (DirectFB; Linux x86_64) Cobalt/4.13031-qa (unlike Gecko) Starboard/1",
        "Mozilla/5.0(SMART-TV; Linux; Tizen 4.0.0.2) AppleWebkit/605.1.15 (KHTML, like Gecko) SamsungBrowser/9.2 TV Safari/605.1.15"};
    return profiles[index<3?index:0];
}
bool trustedTvScript(const std::string& url) {
    const std::string prefix="https://www.youtube.com/s/";
    if(url.compare(0,prefix.size(),prefix) || url.size()>8192 || url.find_first_of("\r\n\t \\#")!=std::string::npos) return false;
    // Admit bounded YouTube TV/kabuki loader modules as well as base/main.
    return url.find("/kabuki/")!=std::string::npos || url.find("/tv/")!=std::string::npos ||
        url.find("kabuki.")!=std::string::npos || url.find("m=base")!=std::string::npos || url.find("m=main")!=std::string::npos;
}
std::string tvBootstrapKind(const std::string& document) {
    if(document.find("consent.youtube.com")!=std::string::npos) return "consent_page";
    if(document.find("unusual traffic")!=std::string::npos || document.find("/sorry/")!=std::string::npos) return "request_rejected";
    if(document.find("unsupported")!=std::string::npos || document.find("not supported")!=std::string::npos) return "unsupported_client_page";
    return document.find("<script")!=std::string::npos?"script_page":"unrecognized_page";
}
std::vector<std::string> tvScriptCandidates(const std::string& document) {
    std::vector<std::string> urls;if(document.empty() || document.size()>16*1024*1024) return urls;
    const auto add=[&](std::string value,bool preferred) {
        value=normalize(value);if(!trustedTvScript(value)) return;
        for(const auto& existing:urls) if(existing==value) return;
        if(urls.size()>=8) return;
        if(preferred) urls.insert(urls.begin(),value);else urls.push_back(value);
    };
    for(size_t p=0;(p=document.find("<script",p))!=std::string::npos;) {
        const auto end=document.find('>',p);if(end==std::string::npos) break;const auto tag=document.substr(p,end-p+1);p=end+1;
        std::string id,src;if(attribute(tag,"src",src)) {attribute(tag,"id",id);add(src,id=="base-js");}
    }
    for(size_t p=0;p<document.size();) {
        if(document[p]!='\'' && document[p]!='"') {++p;continue;}std::string value;const auto before=p;
        if(!quoted(document,p,value)) {p=before+1;continue;}
        add(value,value.find("m=base")!=std::string::npos || value.find("m=main")!=std::string::npos);
    }
    return urls;
}
bool tvScriptUrl(const std::string& html,std::string& url,std::string& error) {
    url.clear();if(html.empty() || html.size()>4*1024*1024) {error="Invalid TV bootstrap size";return false;}
    const auto urls=tvScriptCandidates(html);if(!urls.empty()) {url=urls.front();return true;}
    error="YouTube TV bootstrap script was not found";return false;
}
bool tvClientPair(const std::string& script,std::string& id,std::string& secret,std::string& error) {
    id.clear();secret.clear();if(script.empty() || script.size()>16*1024*1024) {error="Invalid TV script size";return false;}
    for(size_t at=0;(at=script.find("clientId",at))!=std::string::npos;at+=8) {
        if(at && word((unsigned char)script[at-1])) continue;
        size_t p=at+8;if(p<script.size() && (script[p]=='"' || script[p]=='\'')) ++p;
        spaces(script,p);if(p>=script.size() || script[p++]!=':') continue;spaces(script,p);std::string candidate,second;
        if(!quoted(script,p,candidate)) continue;const std::string suffix=".apps.googleusercontent.com";
        if(candidate.size()<=suffix.size() || candidate.size()>512 || candidate.compare(candidate.size()-suffix.size(),suffix.size(),suffix) || candidate.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.")!=std::string::npos) continue;
        spaces(script,p);if(p>=script.size() || script[p++]!=',') continue;spaces(script,p);
        std::string key;if(p<script.size() && (script[p]=='"' || script[p]=='\'')) {if(!quoted(script,p,key)) continue;}
        else {auto start=p;while(p<script.size() && word((unsigned char)script[p])) ++p;if(p==start) continue;}
        spaces(script,p);if(p>=script.size() || script[p++]!=':') continue;spaces(script,p);
        if(!quoted(script,p,second) || second.empty() || second.size()>512 || second.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos) continue;
        id=candidate;secret=second;return true; // First modern adjacent pair, as upstream.
    }
    for(size_t at=0;(at=script.find("client_id",at))!=std::string::npos;at+=9) {
        if(at && word((unsigned char)script[at-1])) continue;size_t p=at+9;
        if(p<script.size() && (script[p]=='"' || script[p]=='\'')) ++p;spaces(script,p);
        if(p>=script.size() || script[p++]!=':') continue;spaces(script,p);std::string candidate,key,value;
        if(!quoted(script,p,candidate)) continue;const std::string suffix=".apps.googleusercontent.com";
        if(candidate.size()<=suffix.size() || candidate.size()>512 || candidate.compare(candidate.size()-suffix.size(),suffix.size(),suffix) || candidate.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.")!=std::string::npos) continue;
        spaces(script,p);if(p>=script.size() || script[p++]!=',') continue;spaces(script,p);
        if(!quoted(script,p,key) || key!="client_secret") continue;spaces(script,p);if(p>=script.size() || script[p++]!=':') continue;spaces(script,p);
        if(!quoted(script,p,value) || value.empty() || value.size()>512 || value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos) continue;
        id=candidate;secret=value;return true;
    }
    error="YouTube TV OAuth client pattern was not found";return false;
}
std::string legacyTvScript(const std::string& url) {
    auto result=url;auto p=result.find("/dg=0/");if(p!=std::string::npos) result.replace(p,6,"/exm=base/ed=1/");
    p=result.find("/m=base");if(p!=std::string::npos) result.replace(p,7,"/m=main");return result;
}
}
