#pragma once
#include <string>
#include <vector>

namespace smarttube {
struct Json {
    enum Type { Null, Bool, Number, String, Array, Object } type=Null;
    std::string text;
    std::vector<std::string> keys;
    std::vector<Json> children;
    const Json* get(const char* key) const;
    std::string string(const char* key, const char* fallback="") const;
    int integer(const char* key, int fallback=0) const;
};
// Untrusted responses are bounded by bytes, node count and nesting depth.
bool parseJson(const std::string& input, Json& output, std::string& error);
std::string quoteJson(const std::string& input);
}
