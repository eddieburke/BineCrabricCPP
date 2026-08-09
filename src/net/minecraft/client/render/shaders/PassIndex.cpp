#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
namespace net::minecraft::client::render {
namespace {
constexpr std::array kPackProgramIds = {
    PackProgramId{"gbuffers_basic", ""},
    PackProgramId{"gbuffers_line", "gbuffers_basic"},
    PackProgramId{"gbuffers_textured", "gbuffers_basic"},
    PackProgramId{"gbuffers_textured_lit", "gbuffers_textured"},
    PackProgramId{"gbuffers_skybasic", "gbuffers_basic"},
    PackProgramId{"gbuffers_skytextured", "gbuffers_textured"},
    PackProgramId{"gbuffers_clouds", "gbuffers_textured"},
    PackProgramId{"gbuffers_terrain", "gbuffers_textured_lit"},
    PackProgramId{"gbuffers_terrain_solid", "gbuffers_terrain"},
    PackProgramId{"gbuffers_terrain_cutout", "gbuffers_terrain"},
    PackProgramId{"gbuffers_damagedblock", "gbuffers_terrain"},
    PackProgramId{"gbuffers_item", "gbuffers_textured_lit"},
    PackProgramId{"gbuffers_entities", "gbuffers_textured_lit"},
    PackProgramId{"gbuffers_entities_translucent", "gbuffers_entities"},
    PackProgramId{"gbuffers_entities_glowing", "gbuffers_entities"},
    PackProgramId{"gbuffers_lightning", "gbuffers_entities"},
    PackProgramId{"gbuffers_block", "gbuffers_terrain"},
    PackProgramId{"gbuffers_block_translucent", "gbuffers_block"},
    PackProgramId{"gbuffers_spidereyes", "gbuffers_textured"},
    PackProgramId{"gbuffers_armor_glint", "gbuffers_textured"},
    PackProgramId{"gbuffers_beaconbeam", "gbuffers_textured"},
    PackProgramId{"gbuffers_hand", "gbuffers_textured_lit"},
    PackProgramId{"gbuffers_hand_water", "gbuffers_hand"},
    PackProgramId{"gbuffers_weather", "gbuffers_textured_lit"},
    PackProgramId{"gbuffers_water", "gbuffers_terrain"},
    PackProgramId{"gbuffers_particles", "gbuffers_textured_lit"},
    PackProgramId{"gbuffers_particles_translucent", "gbuffers_particles"},
    PackProgramId{"gbuffers_gui", "gbuffers_basic"},
    PackProgramId{"gbuffers_gui_textured", "gbuffers_textured"},
    PackProgramId{"gbuffers_text", "gbuffers_textured"},
    PackProgramId{"shadow", ""},
    PackProgramId{"shadow_solid", "shadow"},
    PackProgramId{"shadow_cutout", "shadow"},
    PackProgramId{"shadow_water", "shadow"},
    PackProgramId{"shadow_entities", "shadow"},
    PackProgramId{"shadow_block", "shadow"},
    PackProgramId{"shadow_lightning", "shadow_entities"}};
std::string programLookupKey(const std::string& programName) {
 const std::size_t hash = programName.find('#');
 return hash == std::string::npos ? programName : programName.substr(0, hash);
}
// shaders.properties scopes `program.X.enabled` under the dimension folder the
// shaders live in (`program.world0/shadowcomp.enabled=false`). Derive that key
// from the program's own source path so the flag actually applies.
std::string dimensionProgramKey(const PackDefinition& definition,
                                const std::string& programName,
                                const std::string& key) {
 const auto program = definition.programs.find(programName);
 if(program == definition.programs.end()) return {};
 const std::array<std::string_view, 6> paths = {program->second.compute, program->second.vertex,
                                                program->second.fragment, program->second.geometry,
                                                program->second.tessControl, program->second.tessEvaluation};
 for(const std::string_view path : paths) {
  if(path.rfind("shaders/", 0) != 0) continue;
  const std::size_t slash = path.find('/', 8);
  if(slash == std::string::npos) continue;
  const std::string folder(path.substr(8, slash - 8));
  if(folder == "lib" || folder == "program") continue;
  return folder + "/" + key;
 }
 return {};
}
bool optionEnabled(const std::string& value) {
 return value != "0" && value != "false";
}
class BoolExpression {
 public:
 explicit BoolExpression(std::string expression,
                         const PackDefinition& definition,
                         const std::unordered_map<std::string, std::string>& settings)
     : text_(std::move(expression)), definition_(definition), settings_(settings) {}
 bool evaluate() {
  skip();
  if(pos_ >= text_.size()) return true;
  const bool value = parseOr();
  skip();
  if(pos_ < text_.size() || failed_) return true;
  return value;
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
  if(token == "true" || token == "1") return true;
  if(token == "false" || token == "0") return false;
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
} // namespace
std::span<const PackProgramId> packProgramIds() {
 return kPackProgramIds;
}
bool isCompositeStageName(const std::string& name) noexcept {
 const std::string_view programName = name;
 for(const std::string_view prefix : kCompositeStagePrefixes) {
  if(!programName.starts_with(prefix)) continue;
  if(programName.size() == prefix.size()) return true;
  const char next = programName[prefix.size()];
  if(next == '_' || (next >= '0' && next <= '9')) return true;
 }
 return false;
}
bool isProgramEnabled(const PackDefinition& definition,
                      const std::unordered_map<std::string, std::string>& settings,
                      const std::string& programName) {
 const std::string key = programLookupKey(programName);
 const auto found = definition.programEnabled.find(key);
 if(found == definition.programEnabled.end()) {
  const std::string prefixed = dimensionProgramKey(definition, programName, key);
  const auto dimFound = prefixed.empty() ? definition.programEnabled.end()
                                         : definition.programEnabled.find(prefixed);
  if(dimFound == definition.programEnabled.end()) {
   return true;
  }
  BoolExpression expression(dimFound->second, definition, settings);
  return expression.evaluate();
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
 const bool enabled = isProgramEnabled(definition, settings, programName);
 cache.emplace(key, enabled);
 return enabled;
}
void indexPackPasses(const PackDefinition& definition,
                     const std::unordered_map<std::string, std::string>& settings,
                     PackPassBuckets& buckets,
                     ProgramEnabledCache& cache) {
 buckets.postPasses.clear();
 buckets.deferredPasses.clear();
 buckets.computePasses.clear();
 buckets.beginPasses.clear();
 buckets.shadowCompositePasses.clear();
 buckets.preparePasses.clear();
 buckets.setupPasses.clear();
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
  return ComputeDispatcher::lessComputeOrder(definition.passes[a], definition.passes[b]);
 });
 std::stable_sort(buckets.setupPasses.begin(), buckets.setupPasses.end(), [&definition](std::size_t a, std::size_t b) {
  return ComputeDispatcher::lessComputeOrder(definition.passes[a], definition.passes[b]);
 });
}
std::string irisShadowProgramForGbuffers(const std::string& gbuffersKey) {
 if(gbuffersKey.rfind("clrwl_", 0) == 0) return {};
 if(gbuffersKey == "gbuffers_terrain_solid") return "shadow_solid";
 if(gbuffersKey == "gbuffers_terrain_cutout") return "shadow_cutout";
 if(gbuffersKey == "gbuffers_water") return "shadow_water";
 if(gbuffersKey.rfind("gbuffers_entities", 0) == 0 || gbuffersKey == "gbuffers_item" ||
    gbuffersKey == "gbuffers_beaconbeam" || gbuffersKey == "gbuffers_armor_glint" ||
    gbuffersKey == "gbuffers_spidereyes" || gbuffersKey == "gbuffers_text") {
  return "shadow_entities";
 }
 if(gbuffersKey == "gbuffers_lightning") return "shadow_lightning";
 if(gbuffersKey.rfind("gbuffers_block", 0) == 0) return "shadow_block";
 if(gbuffersKey.rfind("gbuffers_sky", 0) == 0 || gbuffersKey == "gbuffers_clouds" ||
    gbuffersKey == "gbuffers_hand" || gbuffersKey == "gbuffers_hand_water" ||
    gbuffersKey.rfind("gbuffers_gui", 0) == 0) {
  return {};
 }
 return "shadow";
}
std::string programFallbackKey(const std::string& programKey) {
 const auto found = std::find_if(kPackProgramIds.begin(), kPackProgramIds.end(),
                                 [&programKey](const PackProgramId& id) { return id.name == programKey; });
 if(found != kPackProgramIds.end()) return std::string(found->fallback);
 return {};
}
std::string resolveProgramKey(const PackDefinition& definition,
                              const std::unordered_map<std::string, std::string>& settings,
                              const std::string& requested,
                              ProgramEnabledCache& cache) {
 for(std::string key = requested; !key.empty(); key = programFallbackKey(key)) {
  if(definition.programs.contains(key) && isProgramEnabledCached(definition, settings, key, cache)) return key;
 }
 return {};
}
} // namespace net::minecraft::client::render
