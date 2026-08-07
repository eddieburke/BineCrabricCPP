#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/shaders/PreProcessor.hpp"
#include "net/minecraft/client/render/shaders/ConditionalState.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#include "net/minecraft/client/render/pipeline/Resources.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/block/Block.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <utility>
namespace net::minecraft::client::render {
namespace {
std::string trim(std::string_view value) {
 const std::size_t first = value.find_first_not_of(" \t\r\n");
 if(first == std::string_view::npos) return {};
 const std::size_t last = value.find_last_not_of(" \t\r\n");
 return std::string(value.substr(first, last - first + 1));
}
std::string lowercase(std::string value) {
 for(char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
 return value;
}
bool identifier(std::string_view value) {
 if(value.empty() || (!std::isalpha(static_cast<unsigned char>(value.front())) && value.front() != '_')) return false;
 return std::all_of(value.begin() + 1, value.end(), [](unsigned char ch) { return std::isalnum(ch) || ch == '_'; });
}
bool number(std::string_view value, double& out, bool& integer) {
 std::string text(value);
 if(!text.empty() && (text.back() == 'f' || text.back() == 'F')) text.pop_back();
 char* end = nullptr;
 out = std::strtod(text.c_str(), &end);
 if(end == text.c_str() || *end != '\0' || !std::isfinite(out)) return false;
 integer = text.find_first_of(".eE") == std::string::npos;
 return true;
}
std::string numberText(double value, bool integer) {
 return integer ? std::to_string(static_cast<int>(std::lround(value))) : std::to_string(value);
}
void optionRange(std::string_view comment, PackSetting& setting) {
 const std::size_t open = comment.find('[');
 const std::size_t close = comment.find(']', open == std::string_view::npos ? 0 : open + 1);
 if(open == std::string_view::npos || close == std::string_view::npos) return;
 std::istringstream values{std::string(comment.substr(open + 1, close - open - 1))};
 std::string token;
 bool found = false;
 while(values >> token) {
  double value = 0.0;
  bool ignored = false;
  if(!number(token, value, ignored)) continue;
  if(!found) {
   setting.minimum = value;
   setting.maximum = value;
   found = true;
  } else {
   setting.minimum = std::min(setting.minimum, value);
   setting.maximum = std::max(setting.maximum, value);
  }
 }
 if(found && setting.maximum > setting.minimum) setting.step = (setting.maximum - setting.minimum) / 100.0;
}
void addOption(std::unordered_map<std::string, PackSourceOption>& options,
               std::unordered_set<std::string>& rejected,
               std::string key,
               PackOptionForm form,
               bool enabled,
               std::string_view value,
               std::string_view comment) {
 if(!identifier(key) || rejected.contains(key)) return;
 auto reject = [&options, &rejected](const std::string& name) {
  options.erase(name);
  rejected.insert(name);
 };
 PackSourceOption option;
 option.form = form;
 option.setting.key = std::move(key);
 option.setting.label = option.setting.key;
 if(form == PackOptionForm::Define && value.empty()) {
  option.setting.type = SettingType::Bool;
  option.setting.defaultValue = enabled ? "1" : "0";
  option.setting.step = 1.0;
 } else {
  double parsed = 0.0;
  bool integer = false;
  if(!number(value, parsed, integer)) {
   reject(option.setting.key);
   return;
  }
  option.setting.type = integer ? SettingType::Int : SettingType::Float;
  option.setting.defaultValue = numberText(parsed, integer);
  option.setting.minimum = parsed;
  option.setting.maximum = parsed;
  option.setting.step = integer ? 1.0 : 0.01;
  optionRange(comment, option.setting);
 }
 if(const auto existing = options.find(option.setting.key); existing != options.end()) {
  const bool sameShape = existing->second.form == option.form &&
                         (existing->second.setting.type == SettingType::Bool) ==
                             (option.setting.type == SettingType::Bool);
  if(!sameShape) reject(option.setting.key);
  return;
 }
 options.emplace(option.setting.key, std::move(option));
}
void scanOptions(const std::string& source,
                 std::unordered_map<std::string, PackSourceOption>& options,
                 std::unordered_set<std::string>& rejected) {
 std::istringstream lines(source);
 std::string line;
 int braceDepth = 0;
 while(std::getline(lines, line)) {
  const std::string cleaned = trim(line);
  const bool disabled = cleaned.rfind("//#define", 0) == 0;
  const bool enabled = cleaned.rfind("#define", 0) == 0;
  if(enabled || disabled) {
   const std::string body = trim(std::string_view(cleaned).substr(disabled ? 9 : 7));
   const std::size_t split = body.find_first_of(" \t/");
   const std::string key = body.substr(0, split);
   const std::string rest = split == std::string::npos ? std::string{} : trim(std::string_view(body).substr(split));
   const std::size_t comment = rest.find("//");
   const std::string value = trim(comment == std::string::npos ? rest : rest.substr(0, comment));
   auto reject = [&options, &rejected](const std::string& name) {
    options.erase(name);
    rejected.insert(name);
   };
   if(comment == std::string::npos && !value.empty()) {
    double parsed = 0.0;
    bool integer = false;
    if(!number(value, parsed, integer)) {
     reject(key);
     continue;
    }
   }
   addOption(options,
             rejected,
             key,
             PackOptionForm::Define,
             enabled,
             value,
             comment == std::string::npos ? std::string_view{} : std::string_view(rest).substr(comment + 2));
   continue;
  }
  if(braceDepth != 0 || cleaned.rfind("const ", 0) != 0) {
   braceDepth += static_cast<int>(std::count(line.begin(), line.end(), '{'));
   braceDepth -= static_cast<int>(std::count(line.begin(), line.end(), '}'));
   braceDepth = std::max(0, braceDepth);
   continue;
  }
  const std::size_t equals = cleaned.find('=');
  const std::size_t semicolon = cleaned.find(';', equals == std::string::npos ? 0 : equals + 1);
  if(equals == std::string::npos || semicolon == std::string::npos) continue;
  const std::string left = trim(std::string_view(cleaned).substr(6, equals - 6));
  const std::size_t separator = left.find_last_of(" \t");
  if(separator == std::string::npos) continue;
  const std::string type = trim(std::string_view(left).substr(0, separator));
  if(type != "int" && type != "float") continue;
  const std::string key = trim(std::string_view(left).substr(separator + 1));
  if(key == "shadowMapResolution") continue;
  addOption(options,
            rejected,
            key,
            PackOptionForm::Constant,
            true,
            trim(std::string_view(cleaned).substr(equals + 1, semicolon - equals - 1)),
            std::string_view(cleaned).substr(semicolon + 1));
  braceDepth += static_cast<int>(std::count(line.begin(), line.end(), '{'));
  braceDepth -= static_cast<int>(std::count(line.begin(), line.end(), '}'));
 }
}
void scanShadowMapResolution(const std::string& source, PackDefinition& pack) {
 std::istringstream lines(source);
 std::string line;
 while(std::getline(lines, line)) {
  const std::string cleaned = trim(line);
  if(cleaned.rfind("const int shadowMapResolution", 0) != 0) continue;
  const std::size_t equals = cleaned.find('=');
  const std::size_t semicolon = cleaned.find(';', equals == std::string::npos ? 0 : equals + 1);
  if(equals == std::string::npos || semicolon == std::string::npos) continue;
  double value = 0.0;
  bool integer = false;
  if(number(trim(std::string_view(cleaned).substr(equals + 1, semicolon - equals - 1)), value, integer) && integer) {
   pack.shadowMapResolution = std::clamp(static_cast<int>(std::lround(value)), 0, 16384);
  }
 }
}
void advanceBlockComment(const std::string& line, bool& inBlockComment) {
 for(std::size_t i = 0; i < line.size();) {
  if(inBlockComment) {
   const std::size_t close = line.find("*/", i);
   if(close == std::string::npos) return;
   i = close + 2;
   inBlockComment = false;
   continue;
  }
  if(line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') return;
  if(line[i] == '/' && i + 1 < line.size() && line[i + 1] == '*') {
   inBlockComment = true;
   i += 2;
   continue;
  }
  ++i;
 }
}
std::string firstIdentifier(const std::string& text) {
 std::size_t begin = 0;
 while(begin < text.size() && !isIdentStart(text[begin])) ++begin;
 std::size_t end = begin;
 while(end < text.size() && isIdentChar(text[end])) ++end;
 return text.substr(begin, end - begin);
}
void scanPackConstants(const std::string& source,
                       PackDefinition& pack,
                       const std::unordered_map<std::string, PackSourceOption>& options,
                       const std::unordered_map<std::string, std::string>* values = nullptr) {
 std::istringstream lines(source);
 std::string line;
 PPMacroTable macros;
 seedEngineMacros(pack, macros);
 // The GLSL the driver compiles sees the option-rewritten source, so `#if OPTION`
 // chains must be evaluated with the CURRENT settings, not the shipped defaults,
 // or the engine's shadow tuning and the shader's disagree whenever an option
 // moves (Iris parses programs after option replacement).
 for(const auto& [name, option] : options) {
  const PackSetting& setting = option.setting;
  const std::string effective =
      values != nullptr ? [&] {
       const auto found = values->find(name);
       return found != values->end() ? found->second : setting.defaultValue;
      }()
                        : setting.defaultValue;
  PPMacro macro;
  macro.body = effective;
  const bool trueValue = effective == "1" || effective == "true";
  if(setting.type == SettingType::Bool) {
   if(trueValue) {
    macro.body.clear();
    macros[name] = macro;
   }
  } else {
   macros[name] = macro;
  }
 }
 ConditionalState conditionals(ConditionalState::Flavor::Glsl);
 bool inBlockComment = false;
 while(std::getline(lines, line)) {
  const bool startedInsideComment = inBlockComment;
  advanceBlockComment(line, inBlockComment);
  if(!startedInsideComment) {
   std::string keyword;
   std::string rest;
   if(parseDirective(trim(lineForDirectiveParse(line)), keyword, rest)) {
    if(keyword == "if") {
     conditionals.push(evaluateIfExpression(rest, macros));
    } else if(keyword == "ifdef") {
     conditionals.push(macros.count(firstIdentifier(rest)) != 0);
    } else if(keyword == "ifndef") {
     conditionals.push(macros.count(firstIdentifier(rest)) == 0);
    } else if(keyword == "elif") {
     conditionals.elif(evaluateIfExpression(rest, macros));
    } else if(keyword == "else") {
     conditionals.else_();
    } else if(keyword == "endif") {
     conditionals.endif();
    } else if(keyword == "define" && conditionals.active()) {
     const std::string defName = firstIdentifier(rest);
     if(!options.contains(defName)) {
      parseDefineDirective(rest, macros);
     }
    } else if(keyword == "undef" && conditionals.active()) {
     macros.erase(firstIdentifier(rest));
    }
    continue;
   }
  }
  if(!conditionals.active()) continue;
  const std::string cleaned = trim(line);
  const std::size_t equals = cleaned.find('=');
  const std::size_t semicolon = cleaned.find(';', equals == std::string::npos ? 0 : equals + 1);
  if(equals == std::string::npos || semicolon == std::string::npos) continue;
  const std::string left = trim(std::string_view(cleaned).substr(0, equals));
  const std::string right = trim(std::string_view(cleaned).substr(equals + 1, semicolon - equals - 1));
  const bool on = right == "true";
  if(left == "const bool shadowEntities") {
   pack.shadowEntities = right != "false";
   continue;
  }
  if(left == "const bool shadowHardwareFiltering" || left == "const bool shadowHardwareFiltering0") {
   pack.shadowHardwareFiltering[0] = on;
   if(left == "const bool shadowHardwareFiltering") pack.shadowHardwareFiltering[1] = on;
   continue;
  }
  if(left == "const bool shadowHardwareFiltering1") {
   pack.shadowHardwareFiltering[1] = on;
   continue;
  }
  if(left == "const bool shadowHardwareOffset") {
   pack.shadowHardwareOffset = on;
   continue;
  }
  if(left == "const bool generateShadowColorMipmap") {
   if(on) pack.shadowcolorMipmap[0] = pack.shadowcolorMipmap[1] = true;
   continue;
  }
  for(int index = 0; index < 8; ++index) {
   const std::string suffix = std::to_string(index);
   if(left == "const bool shadowcolor" + suffix + "Mipmap" ||
      left == "const bool shadowColor" + suffix + "Mipmap") {
    pack.shadowcolorMipmap[index] = on;
    continue;
   }
  }
  if(left == "const bool shadowtexNearest") {
   pack.shadowtexNearest[0] = pack.shadowtexNearest[1] = on;
   continue;
  }
  if(left == "const bool shadowtex0Nearest" || left == "const bool shadow0MinMagNearest") {
   pack.shadowtexNearest[0] = on;
   continue;
  }
  if(left == "const bool shadowtex1Nearest" || left == "const bool shadow1MinMagNearest") {
   pack.shadowtexNearest[1] = on;
   continue;
  }
  for(int index = 0; index < 8; ++index) {
   const std::string suffix = std::to_string(index);
   if(left == "const bool shadowcolor" + suffix + "Nearest" ||
      left == "const bool shadowColor" + suffix + "Nearest" ||
      left == "const bool shadowColor" + suffix + "MinMagNearest") {
    pack.shadowcolorNearest[index] = on;
    continue;
   }
  }
  double value = 0.0;
  bool integer = false;
  if(!number(right, value, integer)) continue;
  const float f = static_cast<float>(value);
  if(left == "const float sunPathRotation") {
   pack.sunPathRotation = f;
  } else if(left == "const float wetnessHalflife") {
   pack.wetnessHalflife = std::max(0.0f, f);
  } else if(left == "const float drynessHalflife") {
   pack.drynessHalflife = std::max(0.0f, f);
   } else if(left == "const float centerDepthHalflife") {
    pack.centerDepthHalflife = std::max(0.0f, f);
    pack.usesCenterDepthSmooth = true;
  } else if(left == "const float eyeBrightnessHalflife") {
   pack.eyeBrightnessHalflife = std::max(0.0f, f);
  } else if(left == "const float entityShadowDistanceMul") {
   pack.entityShadowDistanceMul = value >= 0.01 ? f : 0.0f;
  } else if(left == "const float voxelDistance") {
   pack.voxelDistance = std::max(0.0f, f);
  } else if(left == "const float shadowDistance") {
   pack.shadowDistance = std::max(0.0f, f);
  } else if(left == "const float shadowDistanceRenderMul") {
   pack.shadowDistanceRenderMul = f;
  } else if(left == "const float shadowMapFov") {
   pack.shadowMapFov = f;
  } else if(left == "const float shadowNearPlane") {
   pack.shadowNearPlane = f;
  } else if(left == "const float shadowFarPlane") {
   pack.shadowFarPlane = f;
  } else if(left == "const float shadowHardwareOffsetFactor") {
   pack.shadowHardwareOffsetFactor = f;
  } else if(left == "const float shadowHardwareOffsetUnits") {
   pack.shadowHardwareOffsetUnits = f;
  } else if(left == "const float shadowIntervalSize") {
   pack.shadowIntervalSize = std::max(0.0f, f);
  } else if(left == "const float ambientOcclusionLevel") {
   pack.ambientOcclusionLevel = std::clamp(f, 0.0f, 1.0f);
  } else if(left == "const int noiseTextureResolution") {
   pack.noiseTextureResolution = std::clamp(static_cast<int>(value), 1, 4096);
  }
 }
}
std::vector<std::string> scanMipmapEnabled(const std::string& source) {
 std::vector<std::string> buffers;
 for(int index = 0; index < 32; ++index) {
  const std::string name = "colortex" + std::to_string(index) + "MipmapEnabled";
  const std::size_t marker = source.find(name);
  if(marker == std::string::npos) continue;
  const std::size_t equals = source.find('=', marker + name.size());
  const std::size_t semicolon = source.find(';', equals == std::string::npos ? marker : equals + 1);
  if(equals == std::string::npos || semicolon == std::string::npos) continue;
  const std::string value = trim(std::string_view(source).substr(equals + 1, semicolon - equals - 1));
  if(value == "true") buffers.push_back("colortex" + std::to_string(index));
 }
 return buffers;
}
} // namespace
std::string preprocessProperties(const std::string& source,
                                 int mcVersion,
                                 const std::unordered_map<std::string, PackSourceOption>& options,
                                 const std::unordered_map<std::string, std::string>& values) {
 PPMacroTable macros;
 {
  PPMacro mc;
  mc.body = std::to_string(mcVersion);
  macros["MC_VERSION"] = mc;
 }
 for(const auto& [name, option] : options) {
  const PackSetting& setting = option.setting;
  const auto current = values.find(name);
  const std::string& value = current != values.end() ? current->second : setting.defaultValue;
  PPMacro macro;
  macro.body = value;
  const bool trueValue = value == "1" || value == "true";
  if(setting.type == SettingType::Bool) {
   if(trueValue) {
    macro.body.clear();
    macros[name] = macro;
   }
  } else {
   macros[name] = macro;
  }
 }
 std::istringstream lines(source);
 std::string line;
 std::string logical;
 std::string result;
 ConditionalState stack(ConditionalState::Flavor::Properties);
 auto lineActive = [&]() {
  return stack.active();
 };
 while(std::getline(lines, line)) {
  logical += line;
  if(!logical.empty() && logical.back() == '\\') {
   logical.pop_back();
   logical.push_back('\n');
   continue;
  }
  line = std::move(logical);
  logical.clear();
  const std::string cleaned = trim(line);
  std::string keyword;
  std::string rest;
  const bool isDirective = parseDirective(cleaned, keyword, rest);
  if(isDirective) {
   if(keyword == "ifdef" || keyword == "ifndef") {
    const bool defined = macros.count(rest) > 0;
    const bool condition = keyword == "ifdef" ? defined : !defined;
    stack.push(condition);
    continue;
   }
   if(keyword == "if") {
    stack.push(evaluateIfExpression(rest, macros));
    continue;
   }
   if(keyword == "elif") {
    stack.elif(evaluateIfExpression(rest, macros));
    continue;
   }
   if(keyword == "else") {
    stack.else_();
    continue;
   }
   if(keyword == "endif") {
    stack.endif();
    continue;
   }
   if(keyword == "define") {
    if(lineActive()) {
     const std::string name = firstIdentifier(rest);
     if(!options.contains(name)) {
      parseDefineDirective(rest, macros);
     }
    }
    continue;
   }
   if(keyword == "undef") {
    if(lineActive()) macros.erase(trim(rest));
    continue;
   }
   continue;
  }
  if(lineActive() && !cleaned.empty() && cleaned.front() != '#' && cleaned.front() != '!') {
   result += line;
   result.push_back('\n');
  }
 }
 return result;
}
namespace {
struct TransparentStringHash {
 using is_transparent = void;
 std::size_t operator()(std::string_view value) const noexcept {
  return std::hash<std::string_view>{}(value);
 }
};
using ResourceSet = std::unordered_set<std::string, TransparentStringHash, std::equal_to<>>;
template <typename Container>
bool has(const Container& resources, std::string_view path) {
 if constexpr(requires { resources.contains(path); }) {
  return resources.contains(path);
 } else {
  return std::find(resources.begin(), resources.end(), path) != resources.end();
 }
}
void addProgram(PackDefinition& pack,
                const ResourceSet& resources,
                std::string_view key) {
 const std::string path = "shaders/" + std::string(key);
 const std::string fragmentPath = path + ".fsh";
 if(!resources.contains(fragmentPath)) return;
 PackProgramSource source;
 if(resources.contains(path + ".vsh")) source.vertex = path + ".vsh";
 source.fragment = fragmentPath;
 if(resources.contains(path + ".gsh")) source.geometry = path + ".gsh";
 if(resources.contains(path + ".tcs")) source.tessControl = path + ".tcs";
 if(resources.contains(path + ".tes")) source.tessEvaluation = path + ".tes";
  pack.programs.emplace(std::string(key), std::move(source));
}
void noteRenderTargetOutputs(PackDefinition& pack, const std::string& source, bool shadow) {
 if(parseRenderTargetIndices(source).empty()) {
  return;
 }
 for(const std::string& output : renderTargetOutputNames(source)) {
  if(output.rfind("colortex", 0) != 0) {
   continue;
  }
  const int index = std::atoi(output.c_str() + 8) + 1;
  if(shadow) {
   pack.shadowColorBuffers = std::max(pack.shadowColorBuffers, index);
  } else {
   pack.gbufferColorBuffers = std::max(pack.gbufferColorBuffers, index);
  }
 }
}
bool isKnownBufferFormat(const std::string& format) {
 static constexpr std::array<std::string_view, 48> formats = {
     "R8", "R16", "R16F", "R32F", "RG8", "RG16",
     "RG16F", "RG32F", "RGB8", "RGB16", "RGB16F", "RGB32F",
     "R11F_G11F_B10F", "RGB10_A2", "RGBA8", "RGBA16", "RGBA16F", "RGBA32F",
     "RGBA", "R8_SNORM", "R16_SNORM", "RG8_SNORM", "RG16_SNORM", "RGB8_SNORM",
     "RGB16_SNORM", "RGBA8_SNORM", "RGBA16_SNORM", "R8I", "R16I", "R32I",
     "RG8I", "RG16I", "RG32I", "RGB8I", "RGB16I", "RGB32I",
     "RGBA8I", "RGBA16I", "RGBA32I", "R8UI", "R16UI", "R32UI",
     "RG8UI", "RG16UI", "RG32UI", "RGBA8UI", "RGBA16UI", "RGBA32UI"};
 return std::find(formats.begin(), formats.end(), format) != formats.end();
}
bool isOffsetInComment(std::string_view source, std::size_t pos) {
 bool inBlock = false;
 for(std::size_t i = 0; i < pos && i < source.size();) {
  if(!inBlock && i + 1 < source.size() && source[i] == '/' && source[i + 1] == '/') {
   const std::size_t lineEnd = source.find('\n', i + 2);
   if(pos < (lineEnd == std::string_view::npos ? source.size() : lineEnd)) {
    return true;
   }
   i = lineEnd == std::string_view::npos ? source.size() : lineEnd + 1;
   continue;
  }
  if(!inBlock && i + 1 < source.size() && source[i] == '/' && source[i + 1] == '*') {
   inBlock = true;
   i += 2;
   continue;
  }
  if(inBlock && i + 1 < source.size() && source[i] == '*' && source[i + 1] == '/') {
   inBlock = false;
   i += 2;
   continue;
  }
  ++i;
 }
 return inBlock;
}
void scanTargetDirective(PackDefinition& pack,
                         const std::string& source,
                         const std::string& name,
                         const std::string& targetKey) {
 constexpr std::size_t npos = std::string::npos;
 auto findAssign = [&](const std::string& key) -> std::pair<std::size_t, std::size_t> {
  for(std::size_t search = 0;;) {
   const std::size_t marker = source.find(key, search);
   if(marker == npos) return {npos, npos};
   if(isOffsetInComment(source, marker)) {
    search = marker + 1;
    continue;
   }
   const std::size_t after = marker + key.size();
   if(after < source.size() && (std::isalnum(static_cast<unsigned char>(source[after])) || source[after] == '_')) {
    search = after;
    continue;
   }
   const std::size_t equals = source.find('=', after);
   const std::size_t semicolon = source.find(';', equals == npos ? after : equals + 1);
   if(equals == npos || semicolon == npos) return {npos, npos};
   return {equals, semicolon};
  }
 };
 if(const auto [eq, sc] = findAssign(name + "Format"); eq != npos) {
  const std::string format = trim(std::string_view(source).substr(eq + 1, sc - eq - 1));
  if(isKnownBufferFormat(format)) pack.targets[targetKey].format = format == "RGBA" ? "RGBA8" : format;
 }
 if(const auto [eq, sc] = findAssign(name + "Clear"); eq != npos) {
  const std::string value = trim(std::string_view(source).substr(eq + 1, sc - eq - 1));
  if(value == "true" || value == "false") pack.targets[targetKey].clear = value == "true";
 }
 if(const std::size_t marker = source.find(name + "ClearColor"); marker != npos) {
  if(!isOffsetInComment(source, marker)) {
   const std::size_t open = source.find('(', marker);
   const std::size_t close = source.find(')', open == npos ? marker : open + 1);
   if(open != npos && close != npos) {
    std::string values = source.substr(open + 1, close - open - 1);
    std::replace(values.begin(), values.end(), ',', ' ');
    std::istringstream stream(values);
    float parsed[4]{};
    if(stream >> parsed[0] >> parsed[1] >> parsed[2] >> parsed[3]) {
     std::copy(std::begin(parsed), std::end(parsed), pack.targets[targetKey].clearColor);
     pack.targets[targetKey].customClearColor = true;
    }
   }
  }
 }
}
void inferColortexFormatsFromLayouts(PackDefinition& pack, const std::string& source);
void scanTargetFormats(PackDefinition& pack, const std::string& source) {
 static constexpr std::array<std::string_view, 8> legacyTargets = {
     "gcolor", "gdepth", "gnormal", "composite", "gaux1", "gaux2", "gaux3", "gaux4"};
 for(int index = 0; index < 32; ++index) {
  const std::string key = "colortex" + std::to_string(index);
  scanTargetDirective(pack, source, key, key);
  if(index < static_cast<int>(legacyTargets.size())) {
   scanTargetDirective(pack, source, std::string(legacyTargets[static_cast<std::size_t>(index)]), key);
  }
 }
 for(int index = 0; index < 8; ++index) {
  const std::string key = "shadowcolor" + std::to_string(index);
  scanTargetDirective(pack, source, key, key);
 }
 scanTargetDirective(pack, source, "shadowcolor", "shadowcolor");
 inferColortexFormatsFromLayouts(pack, source);
}
void inferColortexFormatsFromLayouts(PackDefinition& pack, const std::string& source) {
 auto upgrade = [&](int index, const std::string& format) {
  if(index < 0 || index >= 32 || !isKnownBufferFormat(format)) return;
  PackTarget& target = pack.targets["colortex" + std::to_string(index)];
  if(target.format.empty() || target.format == "RGBA" || target.format == "RGBA8") {
   target.format = format;
  }
  pack.gbufferColorBuffers = std::max(pack.gbufferColorBuffers, index + 1);
 };
 for(std::size_t search = 0;;) {
  const std::size_t layout = source.find("layout(", search);
  if(layout == std::string::npos) break;
  if(isOffsetInComment(source, layout)) {
   search = layout + 7;
   continue;
  }
  const std::size_t close = source.find(')', layout + 7);
  if(close == std::string::npos) break;
  const std::string quals = lowercase(source.substr(layout + 7, close - layout - 7));
  const std::size_t img = source.find("colorimg", close);
  if(img == std::string::npos || img > close + 160) {
   search = close + 1;
   continue;
  }
  if(isOffsetInComment(source, img)) {
   search = close + 1;
   continue;
  }
  const std::size_t numStart = img + 8;
  if(numStart >= source.size() || !std::isdigit(static_cast<unsigned char>(source[numStart]))) {
   search = close + 1;
   continue;
  }
  const int index = std::atoi(source.c_str() + numStart);
  if(quals.find("rgba32ui") != std::string::npos)
   upgrade(index, "RGBA32UI");
  else if(quals.find("rgba16ui") != std::string::npos)
   upgrade(index, "RGBA16UI");
  else if(quals.find("rgba8ui") != std::string::npos)
   upgrade(index, "RGBA8UI");
  else if(quals.find("rgba32f") != std::string::npos)
   upgrade(index, "RGBA32F");
  else if(quals.find("rgba16f") != std::string::npos)
   upgrade(index, "RGBA16F");
  else if(quals.find("rgba8") != std::string::npos)
   upgrade(index, "RGBA8");
  else if(quals.find("rg8") != std::string::npos)
   upgrade(index, "RG8");
  search = close + 1;
 }
 const auto declaredTypeBefore = [&](std::size_t marker) {
  std::size_t end = marker;
  while(end > 0 && std::isspace(static_cast<unsigned char>(source[end - 1]))) --end;
  std::size_t begin = end;
  while(begin > 0 && (std::isalnum(static_cast<unsigned char>(source[begin - 1])) || source[begin - 1] == '_')) {
   --begin;
  }
  return lowercase(source.substr(begin, end - begin));
 };
 for(std::size_t search = 0;;) {
  const std::size_t marker = source.find("colortex", search);
  if(marker == std::string::npos) break;
  if(isOffsetInComment(source, marker)) {
   search = marker + 8;
   continue;
  }
  const std::size_t numStart = marker + 8;
  if(numStart >= source.size() || !std::isdigit(static_cast<unsigned char>(source[numStart]))) {
   search = marker + 8;
   continue;
  }
  std::size_t numEnd = numStart;
  while(numEnd < source.size() && std::isdigit(static_cast<unsigned char>(source[numEnd]))) ++numEnd;
  if(numEnd < source.size() &&
     (std::isalnum(static_cast<unsigned char>(source[numEnd])) || source[numEnd] == '_')) {
   search = numEnd;
   continue;
  }
  const int index = std::atoi(source.c_str() + numStart);
  const std::string type = declaredTypeBefore(marker);
  if(type == "uint" || type.starts_with("uvec") || type.starts_with("usampler"))
   upgrade(index, "RGBA32UI");
  else if(type == "int" || type.starts_with("ivec") || type.starts_with("isampler"))
   upgrade(index, "RGBA32I");
  else if(type == "float16_t" || type.starts_with("f16vec"))
   upgrade(index, "RGBA16F");
  search = marker + 8;
 }
}
void addPostPrograms(PackDefinition& pack,
                     const ResourceSet& resources,
                     const std::unordered_map<std::string, std::string>& expanded) {
 const std::array<std::string_view, 6> prefixes = kCompositeStagePrefixes;
 for(const std::string_view prefix : prefixes) {
  const int programCount = prefix == "final" ? 1 : 100;
  for(int index = 0; index < programCount; ++index) {
   const std::string key = std::string(prefix) + (index == 0 ? std::string{} : std::to_string(index));
   const std::string path = "shaders/" + key;
   if(!resources.contains(path + ".fsh")) continue;
   const std::string vertex = resources.contains(path + ".vsh") ? path + ".vsh" : std::string{};
   PackProgramSource program;
   program.vertex = vertex;
   program.fragment = path + ".fsh";
   if(resources.contains(path + ".gsh")) program.geometry = path + ".gsh";
   pack.programs.emplace(key, std::move(program));
   PackPass pass;
   pass.name = key;
   pass.type = prefix == "begin" ? "begin" : prefix == "shadowcomp" ? "shadowcomp"
                                         : prefix == "prepare"      ? "prepare"
                                         : prefix == "deferred"     ? "deferred"
                                                                    : "post";
    pass.program = key;
    const std::string source = expanded.at(path + ".fsh");
   pass.outputs = key == "final" ? std::vector<std::string>{"screen"} : renderTargetOutputNames(source);
   pass.mipmapBuffers = scanMipmapEnabled(source);
   if(prefix == "shadowcomp")
    for(std::string& output : pass.outputs) {
     if(output.rfind("colortex", 0) == 0) output.replace(0, 8, "shadowcolor");
     if(output.rfind("shadowcolor", 0) == 0)
      pack.shadowColorBuffers = std::max(pack.shadowColorBuffers, std::atoi(output.c_str() + 11) + 1);
    }
   for(const std::string& output : pass.outputs)
    if(output != "screen") pack.targets.try_emplace(output);
   scanTargetFormats(pack, source);
   pack.passes.push_back(std::move(pass));
  }
 }
}
std::vector<std::string> words(std::string_view value) {
 std::istringstream input{std::string(value)};
 std::vector<std::string> result;
 std::string word;
 while(input >> word) result.push_back(std::move(word));
 return result;
}
bool boolean(std::string_view value, bool& out) {
 const std::string normalized = trim(value);
 if(normalized == "true" || normalized == "1") {
  out = true;
  return true;
 }
 if(normalized == "false" || normalized == "0") {
  out = false;
  return true;
 }
 return false;
}
int legacyRenderTargetIndex(std::string_view name) {
 static constexpr std::array<std::string_view, 8> legacy = {
     "gcolor", "gdepth", "gnormal", "composite", "gaux1", "gaux2", "gaux3", "gaux4"};
 const auto found = std::find(legacy.begin(), legacy.end(), name);
 return found == legacy.end() ? -1 : static_cast<int>(std::distance(legacy.begin(), found));
}
std::vector<std::pair<std::string, std::string>> properties(const std::string& source) {
 std::vector<std::pair<std::string, std::string>> result;
 std::istringstream lines(source);
 std::string line, logical;
 while(std::getline(lines, line)) {
  logical += line;
  if(!logical.empty() && logical.back() == '\\') {
   logical.pop_back();
   continue;
  }
  line = std::move(logical);
  logical.clear();
  const std::string cleaned = trim(line);
  if(cleaned.empty() || cleaned.front() == '#' || cleaned.front() == '!') continue;
  const std::size_t split = cleaned.find_first_of("=:");
  if(split == std::string::npos) continue;
  const std::string key = trim(std::string_view(cleaned).substr(0, split));
  const std::string value = trim(std::string_view(cleaned).substr(split + 1));
  const auto existing = std::find_if(result.begin(), result.end(), [&key](const auto& entry) {
   return entry.first == key;
  });
  if(existing == result.end()) {
   result.emplace_back(key, value);
  } else {
   existing->second = value;
  }
 }
 return result;
}
void readFeatureList(std::set<std::string>& output, std::string_view value) {
 for(std::string feature : words(value)) {
  if(identifier(feature)) {
   for(char& ch : feature) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
   output.insert(std::move(feature));
  }
 }
}
std::vector<int> dimensions(std::string value) {
 for(char& ch : value)
  if(ch == 'x' || ch == 'X' || ch == ',') ch = ' ';
 std::vector<int> result;
 for(const std::string& token : words(value)) {
  char* end = nullptr;
  const long parsed = std::strtol(token.c_str(), &end, 10);
  if(end == token.c_str() || *end != '\0' || parsed <= 0 || parsed > 16384) return {};
  result.push_back(static_cast<int>(parsed));
 }
 return result;
}
void seedProfiles(PackDefinition& pack, const std::string& source) {
 pack.profiles.clear();
 for(const auto& [key, value] : properties(source)) {
  if(key.rfind("profile.", 0) != 0) continue;
  PackProfile profile;
  profile.name = key.substr(8);
  for(const std::string& token : words(value)) {
   if(token.rfind("!program.", 0) == 0) {
    const std::string program = token.substr(9);
    if(!program.empty()) profile.disabledPrograms.push_back(program);
    continue;
   }
   if(token.rfind("profile.", 0) == 0) continue;
   if(!token.empty() && token.front() == '!') {
    const std::string option = token.substr(1);
    if(!option.empty()) profile.values[option] = "false";
    continue;
   }
   const std::size_t equals = token.find('=');
   const std::size_t colon = equals == std::string::npos ? token.find(':') : std::string::npos;
   if(equals != std::string::npos) {
    profile.values[token.substr(0, equals)] = token.substr(equals + 1);
    continue;
   }
   if(colon != std::string::npos && colon != 0) {
    profile.values[token.substr(0, colon)] = token.substr(colon + 1);
    continue;
   }
   if(!token.empty()) profile.values[token] = "true";
  }
  if(!profile.name.empty()) pack.profiles.push_back(std::move(profile));
 }
}
void parsePackProperties(PackDefinition& pack, const std::string& source) {
 for(const auto& [key, value] : properties(source)) {
  bool flag = false;
  if(key == "iris.features.required") {
   readFeatureList(pack.requiredFeatures, value);
  } else if(key == "iris.features.optional") {
   readFeatureList(pack.optionalFeatures, value);
  } else if(key == "particles.ordering" && (value == "mixed" || value == "after" || value == "before")) {
   pack.particleOrdering = value;
  } else if(key == "shadow.enabled" && boolean(value, flag)) {
   pack.shadowEnabled = flag;
  } else if(key == "shadowPlayer" && boolean(value, flag)) {
   pack.shadowPlayer = flag;
  } else if(key == "shadowEntities" && boolean(value, flag)) {
   pack.shadowEntities = flag;
  } else if(key == "shadowTerrain" && boolean(value, flag)) {
   pack.shadowTerrain = flag;
  } else if(key == "shadowTranslucent" && boolean(value, flag)) {
   pack.shadowTranslucent = flag;
  } else if(key == "shadowBlockEntities" && boolean(value, flag)) {
   pack.shadowBlockEntities = flag;
  } else if(key == "shadowLightBlockEntities" && boolean(value, flag)) {
   pack.shadowLightBlockEntities = flag;
  } else if(key == "skipAllRendering" && boolean(value, flag)) {
   pack.skipAllRendering = flag;
  } else if(key == "allowConcurrentCompute" && boolean(value, flag)) {
   pack.allowConcurrentCompute = flag;
  } else if(key == "supportsColorCorrection" && boolean(value, flag)) {
   pack.supportsColorCorrection = flag;
  } else if(key == "oldHandLight" && boolean(value, flag)) {
   pack.oldHandLight = flag;
  } else if(key == "voxelizeLightBlocks" && boolean(value, flag)) {
   pack.voxelizeLightBlocks = flag;
  } else if(key == "separateEntityDraws" && boolean(value, flag)) {
   pack.separateEntityDraws = flag;
   pack.particleOrdering = "mixed";
  } else if(key == "oldLighting" && boolean(value, flag)) {
   pack.oldLighting = flag;
  } else if(key == "dynamicHandLight" && boolean(value, flag)) {
   pack.dynamicHandLight = flag;
  } else if(key == "prepareBeforeShadow" && boolean(value, flag)) {
   pack.prepareBeforeShadow = flag;
  } else if(key == "breaksAnisotropy" && boolean(value, flag)) {
   pack.breaksAnisotropy = flag;
  } else if(key == "dhShadow.enabled" && boolean(value, flag)) {
   pack.dhShadowEnabled = flag;
  } else if(key == "fallbackTex") {
   char* end = nullptr;
   const long tex = std::strtol(value.c_str(), &end, 10);
   if(end != value.c_str() && *end == '\0') pack.fallbackTex = static_cast<int>(tex);
  } else if(key == "separateAo" && boolean(value, flag)) {
   pack.separateAo = flag;
  } else if(key == "clouds") {
   const std::string clouds = lowercase(value);
   pack.cloudsMode = clouds;
   pack.renderClouds = clouds != "off" && clouds != "false" && clouds != "0";
  } else if(key == "dhClouds") {
   pack.dhCloudsMode = lowercase(value);
  } else if(key == "particles.before.deferred" && pack.particleOrdering.empty() && boolean(value, flag) && flag) {
   pack.particleOrdering = "before";
  } else if(key == "endFlashShadows" && boolean(value, flag)) {
   pack.endFlashShadows = flag;
  } else if(key == "sun" && boolean(value, flag)) {
   pack.renderSun = flag;
  } else if(key == "moon" && boolean(value, flag)) {
   pack.renderMoon = flag;
  } else if(key == "sky" && boolean(value, flag)) {
   pack.renderSky = flag;
  } else if(key == "stars" && boolean(value, flag)) {
   pack.renderStars = flag;
  } else if(key == "weather") {
   const std::vector<std::string> parts = words(value);
   if(!parts.empty()) {
    pack.renderWeather = parts[0] == "true";
    if(parts.size() > 1) pack.renderWeatherParticles = parts[1] == "true";
   }
  } else if(key == "underwaterOverlay" && boolean(value, flag)) {
   pack.underwaterOverlay = flag;
  } else if(key == "vignette" && boolean(value, flag)) {
   pack.vignette = flag;
  } else if(key == "frustum.culling" && boolean(value, flag)) {
   pack.frustumCulling = flag;
  } else if(key == "occlusion.culling" && boolean(value, flag)) {
   pack.occlusionCulling = flag;
  } else if(key == "rain.depth" && boolean(value, flag)) {
   pack.rainDepth = flag;
  } else if(key == "beacon.beam.depth" && boolean(value, flag)) {
   pack.beaconBeamDepth = flag;
  } else if(key == "backFace.solid" && boolean(value, flag)) {
   pack.backFaceSolid = flag;
  } else if(key == "backFace.cutout" && boolean(value, flag)) {
   pack.backFaceCutout = flag;
  } else if(key == "backFace.cutoutMipped" && boolean(value, flag)) {
   pack.backFaceCutoutMipped = flag;
  } else if(key == "backFace.translucent" && boolean(value, flag)) {
   pack.backFaceTranslucent = flag;
  } else if(key == "sliders") {
   for(const std::string& token : words(value)) {
    if(!token.empty()) pack.sliderKeys.insert(token);
   }
  } else if(key == "screen.columns") {
   char* end = nullptr;
   const long columns = std::strtol(value.c_str(), &end, 10);
   if(end != value.c_str() && *end == '\0') pack.screenColumns = static_cast<int>(columns);
  } else if(key.rfind("screen.", 0) == 0 && key.size() > 15 && key.compare(key.size() - 8, 8, ".columns") == 0) {
   const std::string page = key.substr(7, key.size() - 15);
   char* end = nullptr;
   const long columns = std::strtol(value.c_str(), &end, 10);
   if(!page.empty() && end != value.c_str() && *end == '\0') pack.screenPageColumns[page] = static_cast<int>(columns);
  } else if(key == "screen") {
   pack.screenRoot.clear();
   for(const std::string& token : words(value)) {
    if(!token.empty()) pack.screenRoot.push_back(token);
   }
  } else if(key.rfind("screen.", 0) == 0) {
   const std::string page = key.substr(7);
   std::vector<std::string> tokens;
   for(const std::string& token : words(value)) {
    if(!token.empty()) tokens.push_back(token);
   }
   if(!page.empty()) pack.screenPages[page] = std::move(tokens);
  } else if(key.rfind("profile.", 0) == 0) {
   if(pack.profiles.empty()) seedProfiles(pack, source);
   continue;
  } else if(key.rfind("uniform.", 0) == 0 || key.rfind("variable.", 0) == 0) {
   const bool upload = key.rfind("uniform.", 0) == 0;
   const std::string rest = key.substr(upload ? 8 : 9);
   const std::size_t dot = rest.find('.');
   if(dot == std::string::npos || dot == 0 || dot + 1 >= rest.size()) continue;
   CustomUniformType type{};
   if(!parseCustomUniformType(rest.substr(0, dot), type)) continue;
   CustomUniformDecl decl;
   decl.name = rest.substr(dot + 1);
   decl.type = type;
   decl.upload = upload;
   decl.expression = value;
   if(!decl.name.empty() && !decl.expression.empty()) pack.customUniforms.push_back(std::move(decl));
  } else if(key == "shadow.culling") {
   if(value == "reversed" || value == "safe_zone") {
    pack.shadowCulling = ShadowCullState::SafeZone;
   } else if(boolean(value, flag)) {
    pack.shadowCulling = flag ? ShadowCullState::Advanced : ShadowCullState::Distance;
   }
  } else if(key.rfind("program.", 0) == 0 && key.size() > 16 && key.compare(key.size() - 8, 8, ".enabled") == 0) {
   const std::string programName = key.substr(8, key.size() - 16);
   if(!programName.empty()) {
    pack.programEnabled[programName] = value;
   }
  } else if(key.rfind("size.buffer.", 0) == 0) {
   const std::string bufferName = key.substr(12);
   const std::vector<std::string> fields = words(value);
   if(bufferName.empty() || fields.size() != 2) continue;
   PackTarget& target = pack.targets[bufferName];
   if(fields[0].find('.') == std::string::npos) {
    const int w = std::atoi(fields[0].c_str());
    if(w > 0 && w <= 16384) {
     target.absoluteWidth = w;
     target.scaleX = 1.0f;
    }
   } else {
    const float sx = std::strtof(fields[0].c_str(), nullptr);
    if(sx > 0.0f) target.scaleX = sx;
   }
   if(fields[1].find('.') == std::string::npos) {
    const int h = std::atoi(fields[1].c_str());
    if(h > 0 && h <= 16384) {
     target.absoluteHeight = h;
     target.scaleY = 1.0f;
    }
   } else {
    const float sy = std::strtof(fields[1].c_str(), nullptr);
    if(sy > 0.0f) target.scaleY = sy;
   }
   target.scale = target.scaleX;
  } else if(key.rfind("scale.", 0) == 0) {
   const std::string programName = key.substr(6);
   const std::vector<std::string> fields = words(value);
   if(programName.empty() || fields.empty() || fields.size() > 3) continue;
   ProgramScale scale{};
   scale.scale = std::strtof(fields[0].c_str(), nullptr);
   if(!std::isfinite(scale.scale)) continue;
   if(fields.size() >= 3) {
    scale.offsetX = std::strtof(fields[1].c_str(), nullptr);
    scale.offsetY = std::strtof(fields[2].c_str(), nullptr);
   }
   pack.programScales[programName] = scale;
  } else if(key.rfind("alphaTest.", 0) == 0) {
   const std::string programName = key.substr(10);
   const std::vector<std::string> fields = words(value);
   if(programName.empty() || fields.empty()) continue;
   AlphaTestDirective directive;
   directive.program = programName;
   if(fields.size() == 1 && (lowercase(fields[0]) == "off" || lowercase(fields[0]) == "false")) {
    directive.enabled = false;
    directive.func = "ALWAYS";
    directive.ref = 0.0f;
   } else if(fields.size() >= 2) {
    directive.enabled = true;
    directive.func = fields[0];
    directive.ref = std::strtof(fields[1].c_str(), nullptr);
   } else {
    continue;
   }
   pack.alphaTests.push_back(std::move(directive));
  } else if(key.rfind("flip.", 0) == 0) {
   const std::string binding = key.substr(5);
   const std::size_t separator = binding.find('.');
   if(separator != std::string::npos && boolean(value, flag))
    pack.flips[binding] = flag;
  } else if(key.rfind("indirect.", 0) == 0) {
   const std::vector<std::string> fields = words(value);
   if(fields.size() != 2) continue;
   char* end = nullptr;
   const long buffer = std::strtol(fields[0].c_str(), &end, 10);
   char* offsetEnd = nullptr;
   const long long offset = std::strtoll(fields[1].c_str(), &offsetEnd, 10);
   if(end == fields[0].c_str() || *end != '\0' || offsetEnd == fields[1].c_str() || *offsetEnd != '\0' ||
      buffer < 0 || buffer >= kMaxShaderStorageBuffers || offset < 0 || offset % 4 != 0)
    continue;
   pack.indirectDispatches[key.substr(9)] = {static_cast<int>(buffer), static_cast<std::size_t>(offset)};
  } else if(key == "texture.noise" || key.rfind("customTexture.", 0) == 0 || key.rfind("texture.", 0) == 0) {
   std::vector<std::string> fields = words(value);
   if(fields.empty() || pack.customTextures.size() >= 32) continue;
   CustomTexture texture;
   if(key == "texture.noise") {
    texture.name = "noisetex";
   } else if(key.rfind("customTexture.", 0) == 0) {
    texture.name = key.substr(14);
   } else {
    const std::string binding = key.substr(8);
    const std::size_t separator = binding.find('.');
    if(separator == std::string::npos) continue;
    texture.stage = binding.substr(0, separator);
    texture.name = binding.substr(separator + 1);
    const std::size_t samplerDot = texture.name.find('.');
    if(samplerDot != std::string::npos) texture.name = texture.name.substr(0, samplerDot);
    if(texture.stage != "setup" && texture.stage != "begin" && texture.stage != "shadowcomp" &&
       texture.stage != "prepare" && texture.stage != "gbuffers" && texture.stage != "deferred" &&
       texture.stage != "composite") continue;
   }
   texture.path = fields[0];
   if(fields.size() == 1) {
    texture.encoded = true;
    texture.type = "TEXTURE_2D";
    texture.internalFormat = "RGBA8";
    texture.pixelFormat = "RGBA";
    texture.pixelType = "UNSIGNED_BYTE";
   } else {
    if(fields.size() < 3) continue;
    texture.type = fields[1];
    texture.internalFormat = fields[2];
    const std::size_t count = lowercase(texture.type).find("1d") != std::string::npos
                                  ? 1
                              : lowercase(texture.type).find("3d") != std::string::npos ? 3
                                                                                        : 2;
    if(fields.size() != 5 + count) continue;
    for(std::size_t i = 0; i < count; ++i) {
     const std::vector<int> parsed = dimensions(fields[3 + i]);
     if(parsed.size() != 1) {
      texture.dimensions.clear();
      break;
     }
     texture.dimensions.push_back(parsed.front());
    }
    texture.pixelFormat = fields[3 + count];
    texture.pixelType = fields[4 + count];
   }
   if(identifier(texture.name) && (texture.encoded || !texture.dimensions.empty()))
    pack.customTextures.push_back(std::move(texture));
  } else if(key.rfind("bufferObject.", 0) == 0) {
   const std::string indexText = key.substr(13);
   char* end = nullptr;
   const long index = std::strtol(indexText.c_str(), &end, 10);
   const std::vector<std::string> fields = words(value);
   if(end == indexText.c_str() || *end != '\0' || index < 0 || index >= kMaxShaderStorageBuffers ||
      fields.empty())
    continue;
   char* sizeEnd = nullptr;
   const unsigned long long size = std::strtoull(fields[0].c_str(), &sizeEnd, 10);
   if(sizeEnd == fields[0].c_str() || *sizeEnd != '\0' || size == 0) continue;
   BufferObject buffer;
   buffer.index = static_cast<int>(index);
   buffer.byteSize = static_cast<std::size_t>(size);
   if(fields.size() <= 2) {
    if(fields.size() == 2) buffer.initPath = fields[1];
   } else {
    if(fields.size() < 4) continue;
    buffer.relative = lowercase(fields[1]) == "true";
    char* xEnd = nullptr;
    buffer.scaleX = std::strtof(fields[2].c_str(), &xEnd);
    char* yEnd = nullptr;
    buffer.scaleY = std::strtof(fields[3].c_str(), &yEnd);
    if(xEnd == fields[2].c_str() || *xEnd != '\0' || yEnd == fields[3].c_str() || *yEnd != '\0') continue;
   }
   for(std::size_t i = 0; i < pack.bufferObjects.size();) {
    if(pack.bufferObjects[i].index == buffer.index) {
     pack.bufferObjects.erase(pack.bufferObjects.begin() + static_cast<std::ptrdiff_t>(i));
    } else {
     ++i;
    }
   }
   pack.bufferObjects.push_back(buffer);
  } else if(key.rfind("image.", 0) == 0) {
   const std::vector<std::string> fields = words(value);
   if(fields.size() < 7 || fields.size() > 9 || pack.images.size() >= 16) continue;
   CustomImage image;
   image.name = key.substr(6);
   image.sampler = fields[0];
   image.format = fields[1];
   image.internalFormat = fields[2];
   image.pixelType = fields[3];
   if((image.sampler != "none" && !identifier(image.sampler)) ||
      !boolean(fields[4], image.clearEachFrame) || !boolean(fields[5], image.relative)) continue;
   if(image.sampler == "none") image.sampler.clear();
   if(image.relative) {
    if(fields.size() < 8) continue;
    image.width = std::strtof(fields[6].c_str(), nullptr);
    image.height = std::strtof(fields[7].c_str(), nullptr);
    image.depth = 1;
   } else {
    image.width = std::strtof(fields[6].c_str(), nullptr);
    image.height = fields.size() >= 8 ? std::strtof(fields[7].c_str(), nullptr) : 1.0f;
    image.depth = fields.size() == 9 ? std::max(1, std::atoi(fields[8].c_str())) : 1;
   }
   if(!(image.width > 0.0f) || !(image.height > 0.0f) || image.depth > 16384) continue;
   pack.images.push_back(std::move(image));
  } else if(key.rfind("blend.", 0) == 0) {
   const std::vector<std::string> fields = words(value);
   if(fields.size() != 4 && !(fields.size() == 1 && lowercase(fields[0]) == "off")) continue;
   const std::string target = key.substr(6);
   const std::size_t separator = target.find_last_of('.');
   BufferBlend blend;
   blend.program = separator == std::string::npos ? target : target.substr(0, separator);
   if(separator != std::string::npos) {
    const std::string buffer = target.substr(separator + 1);
    if(buffer.rfind("colortex", 0) == 0) {
     blend.buffer = std::atoi(buffer.c_str() + 8);
    } else {
     const int legacy = legacyRenderTargetIndex(buffer);
     if(legacy >= 0) {
      blend.buffer = legacy;
     } else {
      char* end = nullptr;
      const long parsed = std::strtol(buffer.c_str(), &end, 10);
      if(end != buffer.c_str() && *end == '\0') blend.buffer = static_cast<int>(parsed);
     }
    }
   }
   blend.enabled = fields.size() == 4;
   if(blend.enabled) {
    blend.source = fields[0];
    blend.destination = fields[1];
    blend.sourceAlpha = fields[2];
    blend.destinationAlpha = fields[3];
   }
   if(separator != std::string::npos && blend.buffer < 0) continue;
   if(!blend.program.empty() && blend.buffer < 32) pack.bufferBlends.push_back(std::move(blend));
  }
 }
}
void parseDimensionProperties(PackDefinition& pack, const std::string& source) {
 for(const auto& [key, value] : properties(source)) {
  if(key.rfind("dimension.", 0) != 0 || value.empty()) continue;
  const std::string folder = key.substr(10);
  if(!folder.empty() && folder.find_first_of("/\\.") == std::string::npos) pack.dimensionFolders[folder] = value;
 }
}
void parseIdProperties(std::unordered_map<std::string, int>& output,
                       const std::string& source,
                       std::string_view prefix) {
 for(const auto& [key, value] : properties(source)) {
  if(key.rfind(prefix, 0) != 0) continue;
  const std::string idText = key.substr(prefix.size());
  char* end = nullptr;
  const long id = std::strtol(idText.c_str(), &end, 10);
  if(end == idText.c_str() || *end != '\0' || id < std::numeric_limits<int>::min() ||
     id > std::numeric_limits<int>::max()) continue;
  for(std::string name : words(value)) {
   if(name.find('=') != std::string::npos) continue;
   output[lowercase(std::move(name))] = static_cast<int>(id);
  }
 }
}
void parseBlockLayerProperties(PackDefinition& pack, const std::string& source) {
 using net::minecraft::block::Block;
 using namespace net::minecraft::client::render::chunk::terrain_layer;
 for(const auto& [key, value] : properties(source)) {
  int layer = -1;
  if(key == "layer.solid")
   layer = Solid;
  else if(key == "layer.cutout" || key == "layer.cutout_mipped")
   layer = Cutout;
  else if(key == "layer.translucent")
   layer = Translucent;
  else
   continue;
  for(std::string name : words(value)) {
   if(name.empty() || name.front() == '%') continue;
   name = lowercase(std::move(name));
   char* end = nullptr;
   const long numeric = std::strtol(name.c_str(), &end, 10);
   if(end != name.c_str() && *end == '\0' && numeric > 0 && numeric < 256) {
    pack.blockRenderLayers[static_cast<int>(numeric)] = layer;
    continue;
   }
   std::string resolved = name;
   if(resolved.rfind("minecraft:", 0) == 0) resolved.erase(0, 10);
   for(const auto& [mappedName, id] : pack.blockIds) {
    if(mappedName == resolved) pack.blockRenderLayers[id] = layer;
   }
   for(int id = 1; id < Block::BLOCK_COUNT; ++id) {
    Block* block = Block::BLOCKS[static_cast<std::size_t>(id)];
    if(block == nullptr) continue;
    std::string key = lowercase(block->getTranslationKey());
    if(key.rfind("tile.", 0) == 0) key.erase(0, 5);
    if(key == resolved) pack.blockRenderLayers[id] = layer;
   }
  }
 }
}
void addComputePrograms(PackDefinition& pack,
                        const std::vector<std::string>& resources,
                        const PackLoader::ReadText& readText) {
 const auto computePassStage = [](const std::string& name) -> const char* {
  for(const std::string_view stage : kCompositeStagePrefixes) {
   if(ComputeDispatcher::matchesStage(name, stage)) {
    return stage.data();
   }
  }
  if(ComputeDispatcher::matchesStage(name, "setup")) {
   return "setup";
  }
  return nullptr;
 };
 for(const std::string& sourcePath : resources) {
  if(!sourcePath.ends_with(".csh") || sourcePath.rfind("shaders/", 0) != 0 ||
     sourcePath.find('/', 8) != std::string::npos) continue;
  const std::string source = readText(sourcePath);
  PackPass pass;
  pass.name = std::filesystem::path(sourcePath).stem().string();
  const char* stage = computePassStage(pass.name);
  if(stage == nullptr) {
   continue;
  }
  pass.type = std::strcmp(stage, "setup") == 0 ? "setup" : "compute";
  pass.program = pass.name + "#compute";
  pass.order = ComputeDispatcher::computePassOrder(pass.name);
  std::istringstream lines(source);
  std::string line;
  while(std::getline(lines, line)) {
   const std::string cleaned = trim(line);
   for(int axis = 0; axis < 3; ++axis) {
    const std::string key = "local_size_" + std::string(1, static_cast<char>('x' + axis)) + " = ";
    const std::size_t offset = cleaned.find(key);
    if(offset != std::string::npos) pass.localSize[axis] = std::max(1, std::atoi(cleaned.c_str() + offset + key.size()));
   }
   const std::size_t absolute = cleaned.find("const ivec3 workGroups");
   if(absolute != std::string::npos) {
    const std::size_t open = cleaned.find("ivec3(", absolute);
    const std::size_t close = cleaned.find(')', open == std::string::npos ? 0 : open + 6);
    if(open != std::string::npos && close != std::string::npos) {
     const std::vector<int> parsed = dimensions(cleaned.substr(open + 6, close - open - 6));
     if(parsed.size() == 3) {
      for(int axis = 0; axis < 3; ++axis) pass.groups[axis] = parsed[static_cast<std::size_t>(axis)];
      pass.relativeGroups = false;
     }
    }
   }
   const std::size_t relative = cleaned.find("const vec2 workGroupsRender");
   if(relative != std::string::npos) {
    const std::size_t open = cleaned.find("vec2(", relative);
    const std::size_t close = cleaned.find(')', open == std::string::npos ? 0 : open + 5);
    if(open != std::string::npos && close != std::string::npos) {
     std::string values = cleaned.substr(open + 5, close - open - 5);
     std::replace(values.begin(), values.end(), ',', ' ');
     const std::vector<std::string> parsed = words(values);
     if(parsed.size() == 2) {
      pass.groupScale[0] = std::max(0.0f, std::strtof(parsed[0].c_str(), nullptr));
      pass.groupScale[1] = std::max(0.0f, std::strtof(parsed[1].c_str(), nullptr));
      pass.relativeGroups = true;
     }
    }
   }
   constexpr std::string_view sizePrefix = "const ivec3 ";
   if(cleaned.rfind(sizePrefix, 0) == 0) {
    const std::size_t sizeName = cleaned.find("Size");
    const std::size_t open = cleaned.find("ivec3(");
    const std::size_t close = cleaned.find(')', open == std::string::npos ? 0 : open + 6);
    if(sizeName != std::string::npos && open != std::string::npos && close != std::string::npos &&
       pass.relativeGroups) {
     const std::vector<std::string> dims = words(std::string_view(cleaned).substr(open + 6, close - open - 6));
     if(dims.size() == 1 || dims.size() == 3) {
      int size[3] = {1, 1, 1};
      for(int axis = 0; axis < 3; ++axis) {
       size[axis] = std::max(1, std::atoi(dims[dims.size() == 1 ? 0 : static_cast<std::size_t>(axis)].c_str()));
       pass.groups[axis] = (size[axis] + pass.localSize[axis] - 1) / pass.localSize[axis];
      }
      pass.relativeGroups = false;
     }
    }
   }
   const std::string iterationName = "const int " + pass.name + "Iterations";
   if(cleaned.rfind(iterationName, 0) == 0) {
    const std::size_t equals = cleaned.find('=');
    if(equals != std::string::npos) pass.iterations = std::max(1, std::atoi(cleaned.c_str() + equals + 1));
   }
   if(cleaned.rfind("const int computeOrder", 0) == 0) {
    const std::size_t equals = cleaned.find('=');
    if(equals != std::string::npos) {
     pass.order = std::atoi(cleaned.c_str() + equals + 1);
    }
   }
  }
  PackProgramSource program;
  program.compute = sourcePath;
  pack.programs.emplace(pass.program, std::move(program));
  pack.passes.push_back(std::move(pass));
 }
}
void inferVersion(const std::string& source, PackDefinition& pack) {
 const std::size_t version = source.find("#version");
 if(version == std::string::npos) return;
 std::istringstream line(source.substr(version + 8));
 int number = 0;
 line >> number;
 if(number < 110) return;
 const int major = number / 100;
 const int minor = (number / 10) % 10;
 if(major > pack.glslVersionMajor || (major == pack.glslVersionMajor && minor > pack.glslVersionMinor)) {
  pack.glslVersionMajor = major;
  pack.glslVersionMinor = minor;
 }
}
void reprefixProgramPaths(PackDefinition& pack, const std::string& prefix) {
 auto rewrite = [&prefix](std::string& path) {
  if(!path.empty() && path.rfind("shaders/", 0) == 0) path = prefix + path.substr(8);
 };
 for(auto& [name, program] : pack.programs) {
  (void)name;
  rewrite(program.vertex);
  rewrite(program.fragment);
  rewrite(program.compute);
  rewrite(program.geometry);
  rewrite(program.tessControl);
  rewrite(program.tessEvaluation);
 }
}
void loadProgramSet(PackDefinition& out,
                    const std::vector<std::string>& resources,
                    const PackLoader::ReadText& readText,
                    const std::unordered_map<std::string, PackSourceOption>& options,
                    const std::unordered_map<std::string, std::string>& values) {
 std::unordered_map<std::string, std::string> expanded;
 const ResourceSet resourceSet(resources.begin(), resources.end());
 for(const PackProgramId& id : packProgramIds()) addProgram(out, resourceSet, id.name);
 for(const std::string& path : resources) {
  if(!path.ends_with(".fsh")) continue;
  std::string source = resolveShaderIncludes(
      [&](std::string_view file) { return PackLoader::rewriteOptions(readText(file), options, values); },
      path,
      false,
      expanded);
  scanTargetFormats(out, source);
  scanShadowMapResolution(source, out);
  scanPackConstants(source, out, options, &values);
  const std::string name = std::filesystem::path(path).stem().string();
  if(name.rfind("shadowcomp", 0) == 0) continue;
  noteRenderTargetOutputs(out, source, name == "shadow" || name.rfind("shadow_", 0) == 0);
 }
 for(const std::string& path : resources) {
  if(!path.ends_with(".glsl") && !path.ends_with(".vsh") && !path.ends_with(".csh") &&
     !path.ends_with(".gsh") && !path.ends_with(".tcs") && !path.ends_with(".tes")) {
   continue;
  }
  const std::string source = PackLoader::rewriteOptions(readText(path), options, values);
  scanTargetFormats(out, source);
  scanShadowMapResolution(source, out);
  scanPackConstants(source, out, options, &values);
 }
 out.gbufferColorBuffers = std::clamp(out.gbufferColorBuffers, 1, 32);
 auto hasBlendOverride = [&out](const std::string& program) {
  return std::any_of(out.bufferBlends.begin(), out.bufferBlends.end(),
                     [&program](const BufferBlend& blend) { return blend.program == program; });
 };
 for(const PackProgramId& id : packProgramIds()) {
  if(!id.name.starts_with("shadow")) continue;
  const std::string program(id.name);
  if(out.programs.contains(program) && !hasBlendOverride(program)) {
   BufferBlend blend;
   blend.program = program;
   blend.buffer = -1;
   blend.enabled = false;
   out.bufferBlends.push_back(std::move(blend));
  }
 }
 if(const auto spidereyes = out.programs.find("gbuffers_spidereyes");
    spidereyes != out.programs.end() && spidereyes->second.fragment == "shaders/gbuffers_spidereyes.fsh" &&
    !hasBlendOverride("gbuffers_spidereyes")) {
  BufferBlend blend;
  blend.program = "gbuffers_spidereyes";
  blend.buffer = -1;
  blend.enabled = true;
  blend.source = "srcalpha";
  blend.destination = "one";
  blend.sourceAlpha = "zero";
  blend.destinationAlpha = "one";
  out.bufferBlends.push_back(std::move(blend));
 }
 out.shadowColorBuffers = std::clamp(out.shadowColorBuffers, 0, 8);
 if(out.programs.contains("shadow") && out.shadowMapResolution == 0) out.shadowMapResolution = 1024;
 addPostPrograms(out, resourceSet, expanded);
 out.shadowColorBuffers = std::clamp(out.shadowColorBuffers, 0, 8);
 addComputePrograms(out, resources, readText);
}
bool dimensionSetHasPrograms(const PackDefinition& pack) {
 return std::any_of(pack.dimensionDefinitions.begin(), pack.dimensionDefinitions.end(),
                    [](const auto& entry) { return entry.second != nullptr && !entry.second->programs.empty(); });
}
} // namespace
bool PackLoader::load(const std::vector<std::string>& resources,
                      const ReadText& readText,
                      PackDefinition& out,
                      std::unordered_map<std::string, PackSourceOption>& options,
                      std::string& error,
                      const std::unordered_map<std::string, std::string>& values) {
 out = PackDefinition{};
 options.clear();
 if(!std::any_of(resources.begin(), resources.end(), [](const std::string& path) { return path.rfind("shaders/", 0) == 0; })) {
  error = "missing shaders directory";
  return false;
 }
 std::unordered_set<std::string> rejectedOptions;
 for(const std::string& path : resources) {
  if(!path.ends_with(".vsh") && !path.ends_with(".fsh") && !path.ends_with(".glsl") && !path.ends_with(".csh") &&
     !path.ends_with(".gsh") && !path.ends_with(".tcs") && !path.ends_with(".tes")) continue;
  const std::string source = readText(path);
  inferVersion(source, out);
  scanOptions(source, options, rejectedOptions);
 }
 const bool customDimensionProperties =
     has(resources, "shaders/dimension.properties") || has(resources, "dimension.properties");
 if(has(resources, "shaders/shaders.properties")) {
  seedProfiles(out, readText("shaders/shaders.properties"));
 }
 const auto preprocessProps = [&](const std::string& src) {
  return preprocessProperties(src, 10703, options, values);
 };
 if(has(resources, "shaders/shaders.properties"))
  parsePackProperties(out, preprocessProps(readText("shaders/shaders.properties")));
 if(has(resources, "shaders/dimension.properties"))
  parseDimensionProperties(out, preprocessProps(readText("shaders/dimension.properties")));
 else if(has(resources, "dimension.properties"))
  parseDimensionProperties(out, preprocessProps(readText("dimension.properties")));
 if(has(resources, "shaders/entity.properties"))
  parseIdProperties(out.entityIds, preprocessProps(readText("shaders/entity.properties")), "entity.");
 if(has(resources, "shaders/item.properties"))
  parseIdProperties(out.itemIds, preprocessProps(readText("shaders/item.properties")), "item.");
 if(has(resources, "shaders/block.properties")) {
  const std::string blockProps = preprocessProps(readText("shaders/block.properties"));
  parseIdProperties(out.blockIds, blockProps, "block.");
  parseBlockLayerProperties(out, blockProps);
  out.hasBlockProperties = true;
 }
 if(!out.requiredFeatures.contains("SSBO") && !out.optionalFeatures.contains("SSBO") &&
    !out.bufferObjects.empty()) {
  error = "An SSBO is being used, but the feature flag for SSBO's hasn't been set! Please set either a "
          "requirement or check for the SSBO feature using \"iris.features.required/optional = ssbo\".";
  return false;
 }
 if(!out.requiredFeatures.contains("CUSTOM_IMAGES") && !out.optionalFeatures.contains("CUSTOM_IMAGES") &&
    !out.images.empty()) {
  error = "Custom images are being used, but the feature flag for custom images hasn't been set! Please set "
          "either a requirement or check for custom images' feature flag using "
          "\"iris.features.required/optional = CUSTOM_IMAGES\".";
  return false;
 }
 for(const std::string& feature : out.requiredFeatures) {
  if(!featureSupported(feature)) {
   error = "required feature '" + feature + "' is unsupported on this system";
   return false;
  }
 }
 loadProgramSet(out, resources, readText, options, values);
 if(has(resources, "shaders/lang/en_us.lang") || has(resources, "shaders/lang/en_US.lang")) {
  const std::string langPath =
      has(resources, "shaders/lang/en_us.lang") ? "shaders/lang/en_us.lang" : "shaders/lang/en_US.lang";
  for(const auto& [key, value] : properties(readText(langPath))) {
   if(key.rfind("option.", 0) == 0) {
    const std::string rest = key.substr(7);
    if(rest.ends_with(".comment")) {
     const std::string optionKey = rest.substr(0, rest.size() - 8);
     const auto option = options.find(optionKey);
     if(option != options.end()) option->second.setting.comment = value;
     continue;
    }
    const auto option = options.find(rest);
    if(option != options.end()) option->second.setting.label = value;
    continue;
   }
   if(key.rfind("prefix.", 0) == 0) {
    const auto option = options.find(key.substr(7));
    if(option != options.end()) option->second.setting.valuePrefix = value;
    continue;
   }
   if(key.rfind("suffix.", 0) == 0) {
    const auto option = options.find(key.substr(7));
    if(option != options.end()) option->second.setting.valueSuffix = value;
    continue;
   }
   if(key.rfind("value.", 0) == 0) {
    const std::string rest = key.substr(6);
    const std::size_t dot = rest.find('.');
    if(dot == std::string::npos || dot == 0 || dot + 1 >= rest.size()) continue;
    const auto option = options.find(rest.substr(0, dot));
    if(option != options.end()) option->second.setting.valueLabels[rest.substr(dot + 1)] = value;
   }
  }
 }
 for(const auto& [key, option] : options) out.settings.push_back(option.setting);
 if(!out.profiles.empty()) {
  PackSetting profile;
  profile.key = "profile";
  profile.type = SettingType::Enum;
  profile.label = "Profile";
  profile.defaultValue = "Default";
  profile.valueOrder.push_back("Default");
  for(const PackProfile& preset : out.profiles) {
   profile.valueOrder.push_back(preset.name);
   profile.valueLabels[preset.name] = preset.name;
  }
  out.settings.push_back(std::move(profile));
 }
 for(PackSetting& setting : out.settings) {
  if(out.sliderKeys.count(setting.key) != 0) setting.asSlider = true;
 }
 std::sort(out.settings.begin(), out.settings.end(), [](const PackSetting& a, const PackSetting& b) { return a.key < b.key; });
 const bool hasWorldN =
     std::any_of(resources.begin(), resources.end(),
                 [](const std::string& path) {
                  return path.rfind("shaders/world-1/", 0) == 0 || path.rfind("shaders/world0/", 0) == 0 ||
                         path.rfind("shaders/world1/", 0) == 0;
                 });
 const bool worldNLegacy = !customDimensionProperties && hasWorldN;
 if(out.dimensionFolders.empty()) {
  if(std::any_of(resources.begin(), resources.end(),
                 [](const std::string& path) { return path.rfind("shaders/world-1/", 0) == 0; }))
   out.dimensionFolders["world-1"] = "minecraft:the_nether";
  if(std::any_of(resources.begin(), resources.end(),
                 [](const std::string& path) { return path.rfind("shaders/world0/", 0) == 0; }))
   out.dimensionFolders["world0"] = "minecraft:overworld";
  if(std::any_of(resources.begin(), resources.end(),
                 [](const std::string& path) { return path.rfind("shaders/world1/", 0) == 0; }))
   out.dimensionFolders["world1"] = "minecraft:the_end";
 }
 for(const auto& [folder, dimensionValue] : out.dimensionFolders) {
  const std::string prefix = "shaders/" + folder + "/";
  std::vector<std::string> mapped;
  for(const std::string& resource : resources) {
   if(resource.rfind(prefix, 0) == 0) mapped.push_back("shaders/" + resource.substr(prefix.size()));
  }
  if(mapped.empty()) continue;
  auto definition = std::make_shared<PackDefinition>();
  std::unordered_map<std::string, PackSourceOption> dimensionOptions;
  std::unordered_set<std::string> dimensionRejected;
  for(const std::string& path : mapped) {
   if(!path.ends_with(".vsh") && !path.ends_with(".fsh") && !path.ends_with(".glsl") && !path.ends_with(".csh") &&
      !path.ends_with(".gsh") && !path.ends_with(".tcs") && !path.ends_with(".tes"))
    continue;
   const std::string source = readText(prefix + path.substr(8));
   inferVersion(source, *definition);
   scanOptions(source, dimensionOptions, dimensionRejected);
  }
  const PackLoader::ReadText dimensionRead =
      [&readText, &prefix](std::string_view path) {
       const std::string normalized(path);
       if(normalized.rfind("shaders/", 0) == 0) {
        const std::string dimPath = prefix + normalized.substr(8);
        const std::string content = readText(dimPath);
        if(!content.empty()) return content;
       }
       return readText(normalized);
      };
  loadProgramSet(*definition, mapped, dimensionRead, dimensionOptions, values);
  if(definition->programs.empty()) continue;
  reprefixProgramPaths(*definition, prefix);
  definition->dimensionFolders.clear();
  definition->dimensionDefinitions.clear();
  for(const std::string& dimension : words(dimensionValue)) {
   if(!dimension.empty()) out.dimensionDefinitions[dimension] = definition;
  }
  for(auto& [name, option] : dimensionOptions) options.try_emplace(name, std::move(option));
 }
 if(worldNLegacy) {
  out.programs.clear();
  out.passes.clear();
 }
 if(out.programs.empty() && !dimensionSetHasPrograms(out)) {
  error = "no usable shader programs";
  return false;
 }
 if(out.programs.empty()) {
  auto pick = [&](const char* key) -> std::shared_ptr<PackDefinition> {
   const auto found = out.dimensionDefinitions.find(key);
   return found != out.dimensionDefinitions.end() ? found->second : nullptr;
  };
  std::shared_ptr<PackDefinition> seed = pick("*");
  if(seed == nullptr) seed = pick("minecraft:overworld");
  if(seed == nullptr) seed = out.dimensionDefinitions.begin()->second;
  if(seed != nullptr) {
   out.programs = seed->programs;
   out.passes = seed->passes;
   out.targets = seed->targets;
   out.gbufferColorBuffers = std::max(out.gbufferColorBuffers, seed->gbufferColorBuffers);
   out.shadowColorBuffers = std::max(out.shadowColorBuffers, seed->shadowColorBuffers);
   if(seed->shadowMapResolution > 0) out.shadowMapResolution = seed->shadowMapResolution;
   if(!seed->customUniforms.empty()) out.customUniforms = seed->customUniforms;
  }
 }
  for(const PackPass& pass : out.passes) {
   if(!pass.mipmapBuffers.empty()) {
    const int dim = std::max(1024, out.shadowMapResolution > 0 ? out.shadowMapResolution : 2048);
    int level = 0;
    for(int d = dim; d > 1; d >>= 1) ++level;
    out.mcMipmapLevel = std::max(out.mcMipmapLevel, level);
    break;
   }
  }
  auto scanCenterDepth = [](PackDefinition& definition) {
   if(definition.usesCenterDepthSmooth) return;
   for(const auto& [name, source] : definition.programs) {
    (void)name;
    if(source.vertex.find("centerDepthSmooth") != std::string::npos ||
       source.fragment.find("centerDepthSmooth") != std::string::npos ||
       source.compute.find("centerDepthSmooth") != std::string::npos ||
       source.geometry.find("centerDepthSmooth") != std::string::npos ||
       source.tessControl.find("centerDepthSmooth") != std::string::npos ||
       source.tessEvaluation.find("centerDepthSmooth") != std::string::npos) {
     definition.usesCenterDepthSmooth = true;
     return;
    }
   }
  };
  scanCenterDepth(out);
  for(const auto& [dimension, definition] : out.dimensionDefinitions) {
   (void)dimension;
   scanCenterDepth(*definition);
  }
  return true;
}
std::string PackLoader::rewriteOptions(const std::string& source,
                                       const std::unordered_map<std::string, PackSourceOption>& options,
                                       const std::unordered_map<std::string, std::string>& values) {
 std::istringstream lines(source);
 std::string result;
 std::string line;
 while(std::getline(lines, line)) {
  const std::string cleaned = trim(line);
  const bool disabled = cleaned.rfind("//#define", 0) == 0;
  const bool enabled = cleaned.rfind("#define", 0) == 0;
  bool replaced = false;
  if(enabled || disabled) {
   const std::string body = trim(std::string_view(cleaned).substr(disabled ? 9 : 7));
   const std::size_t split = body.find_first_of(" \t/");
   const std::string key = body.substr(0, split);
   const std::string tail = split == std::string::npos ? std::string{} : trim(std::string_view(body).substr(split));
   const std::size_t comment = tail.find("//");
   const std::string macroBody =
       trim(comment == std::string::npos ? std::string_view(tail) : std::string_view(tail).substr(0, comment));
   const auto option = options.find(key);
   const auto value = values.find(key);
   if(option != options.end() && value != values.end() && option->second.form == PackOptionForm::Define) {
    const bool boolean = option->second.setting.type == SettingType::Bool;
    if(boolean == macroBody.empty()) {
     const std::string suffix = comment == std::string::npos ? std::string{} : " " + tail.substr(comment);
     line = boolean ? (value->second == "0" ? "//#define " : "#define ") + key + suffix
                    : "#define " + key + " " + value->second + suffix;
     replaced = true;
    }
   }
  }
  if(!replaced && cleaned.rfind("const ", 0) == 0) {
   const std::size_t equals = line.find('=');
   const std::size_t semicolon = line.find(';', equals == std::string::npos ? 0 : equals + 1);
   if(equals != std::string::npos && semicolon != std::string::npos) {
    const std::string left = trim(std::string_view(line).substr(0, equals));
    const std::size_t separator = left.find_last_of(" \t");
    const std::string key = separator == std::string::npos ? std::string{} : trim(std::string_view(left).substr(separator + 1));
    const auto option = options.find(key);
    const auto value = values.find(key);
    if(option != options.end() && value != values.end() && option->second.form == PackOptionForm::Constant) {
     line.replace(equals + 1, semicolon - equals - 1, " " + value->second);
    }
   }
  }
  result += line;
  result += '\n';
 }
 return result;
}
} // namespace net::minecraft::client::render
