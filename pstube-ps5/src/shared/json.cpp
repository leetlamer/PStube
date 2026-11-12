#include "json.h"
#include <limits.h>
#include <stdlib.h>

namespace smarttube {
const Json* Json::get(const char* key) const {
    if(type!=Object) return nullptr;
    for(size_t i=0;i<keys.size();++i) if(keys[i]==key) return &children[i];
    return nullptr;
}
std::string Json::string(const char* key,const char* fallback) const {
    const auto* item=get(key); return item && item->type==String ? item->text : fallback;
}
int Json::integer(const char* key,int fallback) const {
    const auto* item=get(key);
    if(!item || item->type!=Number || item->text.find_first_of(".eE")!=std::string::npos) return fallback;
    char* end=nullptr; const long long value=strtoll(item->text.c_str(),&end,10);
    return end && !*end && value>=INT_MIN && value<=INT_MAX ? (int)value : fallback;
}
namespace {
class Parser {
    const std::string& input;
    size_t position=0, nodes=0;
    std::string& error;
    bool fail(const char* message) { if(error.empty()) error=std::string(message)+" at byte "+std::to_string(position); return false; }
    void space() { while(position<input.size() && (input[position]==' ' || input[position]=='\r' || input[position]=='\n' || input[position]=='\t')) ++position; }
    bool digit() const { return position<input.size() && input[position]>='0' && input[position]<='9'; }
    bool hex(unsigned& out) {
        out=0;
        for(int i=0;i<4;++i) {
            if(position>=input.size()) return fail("Truncated Unicode escape");
            const char c=input[position++];
            unsigned nibble;
            if(c>='0' && c<='9') nibble=c-'0';
            else if(c>='a' && c<='f') nibble=c-'a'+10;
            else if(c>='A' && c<='F') nibble=c-'A'+10;
            else return fail("Invalid Unicode escape");
            out=out*16+nibble;
        }
        return true;
    }
    static void utf8(std::string& out,unsigned point) {
        if(point<0x80) out+=(char)point;
        else if(point<0x800) { out+=(char)(0xc0|(point>>6));out+=(char)(0x80|(point&63)); }
        else if(point<0x10000) { out+=(char)(0xe0|(point>>12));out+=(char)(0x80|((point>>6)&63));out+=(char)(0x80|(point&63)); }
        else { out+=(char)(0xf0|(point>>18));out+=(char)(0x80|((point>>12)&63));out+=(char)(0x80|((point>>6)&63));out+=(char)(0x80|(point&63)); }
    }
    bool string(std::string& out) {
        if(position>=input.size() || input[position++]!='"') return fail("Expected string");
        while(position<input.size()) {
            const unsigned char c=input[position++];
            if(c=='"') return true;
            if(c<32) return fail("Control character in string");
            if(c!='\\') { out+=(char)c;continue; }
            if(position>=input.size()) return fail("Truncated string escape");
            const char escape=input[position++];
            switch(escape) {
            case '"': out+='"';break;case '\\':out+='\\';break;case '/':out+='/';break;
            case 'b':out+='\b';break;case 'f':out+='\f';break;case 'n':out+='\n';break;
            case 'r':out+='\r';break;case 't':out+='\t';break;
            case 'u': {
                unsigned point=0;
                if(!hex(point)) return false;
                if(point>=0xd800 && point<=0xdbff) {
                    if(input.compare(position,2,"\\u")) return fail("Missing low surrogate");
                    position+=2;unsigned low=0;
                    if(!hex(low) || low<0xdc00 || low>0xdfff) return fail("Invalid low surrogate");
                    point=0x10000+((point-0xd800)<<10)+(low-0xdc00);
                } else if(point>=0xdc00 && point<=0xdfff) return fail("Unpaired low surrogate");
                utf8(out,point);break;
            }
            default:return fail("Invalid string escape");
            }
        }
        return fail("Unterminated string");
    }
    bool value(Json& out,unsigned depth) {
        if(depth>64 || ++nodes>250000) return fail("JSON complexity limit exceeded");
        space();if(position>=input.size()) return fail("Missing value");
        const char c=input[position];
        if(c=='"') { out.type=Json::String;return string(out.text); }
        if(c=='{' || c=='[') {
            const bool object=c=='{';out.type=object?Json::Object:Json::Array;
            const char close=object?'}':']';++position;space();
            if(position<input.size() && input[position]==close) { ++position;return true; }
            for(;;) {
                if(object) {
                    std::string key;
                    if(!string(key)) return false;
                    for(const auto& old:out.keys) if(old==key) return fail("Duplicate object key");
                    out.keys.push_back(key);space();
                    if(position>=input.size() || input[position++]!=':') return fail("Missing object colon");
                }
                out.children.emplace_back();
                if(!value(out.children.back(),depth+1)) return false;
                space();if(position>=input.size()) return fail("Unterminated container");
                const char delimiter=input[position++];
                if(delimiter==close) return true;
                if(delimiter!=',') return fail("Missing comma");
                space();
            }
        }
        for(const char* word:{"null","true","false"}) {
            const std::string literal=word;
            if(input.compare(position,literal.size(),literal)==0) {
                out.type=literal=="null"?Json::Null:Json::Bool;
                out.text=literal;position+=literal.size();return true;
            }
        }
        out.type=Json::Number;const size_t begin=position;
        if(c=='-') ++position;
        if(!digit()) return fail("Invalid number");
        if(input[position]=='0') ++position;else while(digit()) ++position;
        if(position<input.size() && input[position]=='.') {
            ++position;if(!digit()) return fail("Missing fraction");while(digit()) ++position;
        }
        if(position<input.size() && (input[position]=='e' || input[position]=='E')) {
            ++position;if(position<input.size() && (input[position]=='+' || input[position]=='-')) ++position;
            if(!digit()) return fail("Missing exponent");while(digit()) ++position;
        }
        out.text=input.substr(begin,position-begin);return true;
    }
public:
    Parser(const std::string& s,std::string& e):input(s),error(e) {}
    bool parse(Json& out) {
        if(input.size()>8*1024*1024) return fail("Response exceeds 8 MiB limit");
        if(!value(out,0)) return false;space();
        return position==input.size() || fail("Trailing JSON data");
    }
};
}
bool parseJson(const std::string& input,Json& output,std::string& error) {
    error.clear();Json parsed;
    if(!Parser(input,error).parse(parsed)) { output={};return false; }
    output=static_cast<Json&&>(parsed);return true;
}
std::string quoteJson(const std::string& input) {
    const char hex[]="0123456789abcdef";
    std::string out="\"";
    for(unsigned char c:input) {
        if(c=='"' || c=='\\') {out+='\\';out+=(char)c;}
        else if(c<32) {out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}
        else out+=(char)c;
    }
    return out+'"';
}
}
