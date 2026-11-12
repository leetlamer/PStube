#pragma once
#include <string>
#include <vector>
namespace smarttube {
const char* tvUserAgent(unsigned index);
// Public YouTube TV bootstrap metadata. Never evaluates downloaded JavaScript.
std::vector<std::string> tvScriptCandidates(const std::string& document);
std::string tvBootstrapKind(const std::string& document);
bool tvScriptUrl(const std::string& html,std::string& url,std::string& error);
bool tvClientPair(const std::string& script,std::string& id,std::string& secret,std::string& error);
bool trustedTvScript(const std::string& url);
std::string legacyTvScript(const std::string& url);
}
