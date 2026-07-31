#include "net/minecraft/client/render/PassIndex.hpp"
#include <algorithm>
#include <cctype>
#include "net/minecraft/client/render/ComputeDispatcher.hpp"
namespace net::minecraft::client::render {
namespace {
std::string programLookupKey(const std::string& programName) {
 const std::size_t hash = programName.find('#');
 return hash == std::string::npos ? programName : programName.substr(0, hash);
}
bool optionEnabled(const std::string& value) {
 return value != "0" && value != "false" && value != "FALSE" && value != "off";
}
class BoolExpression {
 public:
 explicit BoolExpression(std::string expression,
                         const PackDefinition& definition,
                         const std::unordered_map<std::string, std::string>& settings)
     : text_(std::move(expression)), definition_(definition), settings_(settings) {}
 bool evaluate() {
  skip();
  if(pos_ >= text_.size()) return false;
  const bool value = parseOr();
  skip();
  return pos_ >= text_.size() && !failed_ ? value : false;
 }

 private:
 bool parseOr() {
  bool left = parseAnd();
  while(match("||")) left = parseAnd() || left;
  return left;
 }
 bool parseAnd() {
  bool left = parseUnary();
  while(match("&&")) left = parseUnary() && left;
  return left;
 }
 bool parseUnary() {
  skip();
  if(match("!")) return !parseUnary();
  if(match("(")) {
   const bool value = parseOr();
   if(!match(")")) failed_ = true;
   return value;
  }
  return parseAtom();
 }
 bool parseAtom() {
  skip();
  if(pos_ >= text_.size() ||
     (std::isalpha(static_cast<unsigned char>(text_[pos_])) == 0 && text_[pos_] != '_')) {
   failed_ = true;
   return false;
  }
  const std::size_t start = pos_;
  while(pos_ < text_.size() &&
        (std::isalnum(static_cast<unsigned char>(text_[pos_])) != 0 || text_[pos_] == '_')) {
   ++pos_;
  }
  const std::string token = text_.substr(start, pos_ - start);
  if(token == "true" || token == "TRUE" || token == "on" || token == "1") return true;
  if(token == "false" || token == "FALSE" || token == "off" || token == "0") return false;
  if(const auto setting = settings_.find(token); setting != settings_.end()) {
   return optionEnabled(setting->second);
  }
  for(const PackSetting& option : definition_.settings) {
   if(option.key == token) return optionEnabled(option.defaultValue);
  }
  return true;
 }
 void skip() {
  while(pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])) != 0) ++pos_;
 }
 bool match(const char* token) {
  skip();
  const std::size_t len = std::char_traits<char>::length(token);
  if(text_.compare(pos_, len, token) != 0) return false;
  pos_ += len;
  return true;
 }
 std::string text_;
 const PackDefinition& definition_;
 const std::unordered_map<std::string, std::string>& settings_;
 std::size_t pos_ = 0;
 bool failed_ = false;
};
}
bool isProgramEnabled(const PackDefinition& definition,
                      const std::unordered_map<std::string, std::string>& settings,
                      const std::string& programName) {
 const std::string key = programLookupKey(programName);
 const auto found = definition.programEnabled.find(key);
 if(found == definition.programEnabled.end()) {
  return true;
 }
 BoolExpression expression(found->second, definition, settings);
 return expression.evaluate();
}
bool isProgramEnabledCached(const PackDefinition& definition,
                            const std::unordered_map<std::string, std::string>& settings,
                            const std::string& programName,
                            ProgramEnabledCache& cache) {
 const std::string key = programLookupKey(programName);
 if(const auto hit = cache.find(key); hit != cache.end()) return hit->second;
 const bool enabled = isProgramEnabled(definition, settings, key);
 cache.emplace(key, enabled);
 return enabled;
}
void indexPackPasses(const PackDefinition& definition,
                       const std::unordered_map<std::string, std::string>& settings,
                       PackPassBuckets& buckets) {
 buckets.postPasses.clear();
 buckets.deferredPasses.clear();
 buckets.computePasses.clear();
 buckets.beginPasses.clear();
 buckets.shadowCompositePasses.clear();
 buckets.preparePasses.clear();
 buckets.setupPasses.clear();
 ProgramEnabledCache cache;
 for(std::size_t i = 0; i < definition.passes.size(); ++i) {
  const PackPass& pass = definition.passes[i];
  if(pass.program.empty() || !isProgramEnabledCached(definition, settings, pass.program, cache)) {
   continue;
  }
  if(pass.type == "post") {
   buckets.postPasses.push_back(i);
  } else if(pass.type == "deferred") {
   buckets.deferredPasses.push_back(i);
  } else if(pass.type == "compute") {
   buckets.computePasses.push_back(i);
  } else if(pass.type == "begin") {
   buckets.beginPasses.push_back(i);
  } else if(pass.type == "shadowcomp") {
   buckets.shadowCompositePasses.push_back(i);
  } else if(pass.type == "prepare") {
   buckets.preparePasses.push_back(i);
  } else if(pass.type == "setup") {
   buckets.setupPasses.push_back(i);
  }
 }
 std::stable_sort(buckets.computePasses.begin(), buckets.computePasses.end(), [&definition](std::size_t a, std::size_t b) {
  return compute::lessComputeOrder(definition.passes[a], definition.passes[b]);
 });
 std::stable_sort(buckets.setupPasses.begin(), buckets.setupPasses.end(), [&definition](std::size_t a, std::size_t b) {
  return compute::lessComputeOrder(definition.passes[a], definition.passes[b]);
 });
}

std::string irisShadowProgramForGbuffers(const std::string& gbuffersKey) {
 if(gbuffersKey.rfind("clrwl_", 0) == 0) return {};
 if(gbuffersKey == "gbuffers_terrain_solid") return "shadow_solid";
 if(gbuffersKey == "gbuffers_terrain_cutout" || gbuffersKey == "gbuffers_damagedblock") return "shadow_cutout";
 if(gbuffersKey == "gbuffers_water") return "shadow_water";
 if(gbuffersKey.rfind("gbuffers_entities", 0) == 0 || gbuffersKey == "gbuffers_item" ||
    gbuffersKey == "gbuffers_beaconbeam" || gbuffersKey == "gbuffers_lightning") {
  return "shadow_entities";
 }
 if(gbuffersKey.rfind("gbuffers_block", 0) == 0) return "shadow_block";
 return "shadow";
}

std::string resolveIrisShadowProgramKey(const std::string& gbuffersKey,
                                        const std::unordered_map<std::string, PackProgramSource>& programs) {
 std::string programKey = irisShadowProgramForGbuffers(gbuffersKey);
 if(programKey.empty()) return {};
 if(!programs.contains(programKey)) programKey = "shadow";
 return programKey;
}
}
