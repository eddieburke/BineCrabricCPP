#include "net/minecraft/client/render/shaderpack/ShaderPassScheduler.hpp"
#include <algorithm>
#include <cctype>
#include "net/minecraft/client/render/shaderpack/ComputeDispatcher.hpp"
namespace net::minecraft::client::render::shaderpack {
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
                         const ShaderPackDefinition& definition,
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
 const ShaderPackDefinition& definition_;
 const std::unordered_map<std::string, std::string>& settings_;
 std::size_t pos_ = 0;
 bool failed_ = false;
};
} // namespace
bool isProgramEnabled(const ShaderPackDefinition& definition,
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
void indexShaderPasses(const ShaderPackDefinition& definition,
                       const std::unordered_map<std::string, std::string>& settings,
                       ShaderPassBuckets& buckets) {
 buckets.postPasses.clear();
 buckets.deferredPasses.clear();
 buckets.computePasses.clear();
 buckets.beginPasses.clear();
 buckets.shadowCompositePasses.clear();
 buckets.preparePasses.clear();
 buckets.setupPasses.clear();
 for(std::size_t i = 0; i < definition.passes.size(); ++i) {
  const ShaderPass& pass = definition.passes[i];
  if(pass.program.empty() || !isProgramEnabled(definition, settings, pass.program)) {
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
  return ComputeDispatcher::lessComputeOrder(definition.passes[a], definition.passes[b]);
 });
 std::stable_sort(buckets.setupPasses.begin(), buckets.setupPasses.end(), [&definition](std::size_t a, std::size_t b) {
  return ComputeDispatcher::lessComputeOrder(definition.passes[a], definition.passes[b]);
 });
}
} // namespace net::minecraft::client::render::shaderpack
