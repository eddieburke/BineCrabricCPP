#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include <cstdio>
#include <mutex>
#include <sstream>
#include <string_view>
#include <utility>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaderpack/BiomeTables.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/ColorWheelMerge.hpp"
#include "net/minecraft/client/render/shaders/ConditionalState.hpp"
#include "net/minecraft/client/render/shaders/CoreGlslTransformer.hpp"
#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/shaders/PreProcessor.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include <algorithm>
#include <array>
namespace net::minecraft::client::render {
namespace {
using PackCatalog::lower;
struct GlShaderSnapshot {
 int glVersion = 330;
 int glslVersion = 330;
 int colorBuffers = 1;
 std::string vendorMacro = "MC_GL_VENDOR_OTHER";
 std::string rendererMacro = "MC_GL_RENDERER_OTHER";
 std::vector<std::string> extensions;
 bool captured = false;
};
GlShaderSnapshot gSnapshot;
std::mutex gSnapshotMutex;
void captureDriverMacros(std::string& outVendor, std::string& outRenderer) {
 const auto text = [](unsigned int name) {
  const char* value = reinterpret_cast<const char*>(::glGetString(name));
  return lower(value == nullptr ? std::string{} : std::string(value));
 };
 const std::string vendorText = text(0x1F00);
 const std::string rendererText = text(0x1F01);
 const std::string& vendor = vendorText;
 const std::string& renderer = rendererText;
 const auto starts = [](std::string_view value, std::string_view prefix) { return value.starts_with(prefix); };
 std::string vendorMacro = "MC_GL_VENDOR_OTHER";
 if(starts(vendor, "ati"))
  vendorMacro = "MC_GL_VENDOR_ATI";
 else if(starts(vendor, "intel"))
  vendorMacro = "MC_GL_VENDOR_INTEL";
 else if(starts(vendor, "nvidia"))
  vendorMacro = "MC_GL_VENDOR_NVIDIA";
 else if(starts(vendor, "amd"))
  vendorMacro = "MC_GL_VENDOR_AMD";
 else if(starts(vendor, "x.org"))
  vendorMacro = "MC_GL_VENDOR_XORG";
 std::string rendererMacro = "MC_GL_RENDERER_OTHER";
 if(starts(renderer, "amd") || starts(renderer, "ati") || starts(renderer, "radeon"))
  rendererMacro = "MC_GL_RENDERER_RADEON";
 else if(starts(renderer, "gallium"))
  rendererMacro = "MC_GL_RENDERER_GALLIUM";
 else if(starts(renderer, "intel"))
  rendererMacro = "MC_GL_RENDERER_INTEL";
 else if(starts(renderer, "geforce") || starts(renderer, "nvidia"))
  rendererMacro = "MC_GL_RENDERER_GEFORCE";
 else if(starts(renderer, "quadro") || starts(renderer, "nvs"))
  rendererMacro = "MC_GL_RENDERER_QUADRO";
 else if(starts(renderer, "mesa"))
  rendererMacro = "MC_GL_RENDERER_MESA";
 else if(starts(renderer, "apple"))
  rendererMacro = "MC_GL_RENDERER_APPLE";
 outVendor = std::move(vendorMacro);
 outRenderer = std::move(rendererMacro);
}
template <std::size_t N>
void appendIndexedMacros(std::vector<ShaderMacro>& out,
                         std::string_view prefix,
                         const std::array<std::string_view, N>& names) {
 for(std::size_t index = 0; index < names.size(); ++index)
  out.push_back({std::string(prefix) + std::string(names[index]), std::to_string(index)});
}
const char* hostOsMacro() {
#if defined(_WIN32)
 return "MC_OS_WINDOWS";
#elif defined(__APPLE__)
 return "MC_OS_MAC";
#elif defined(__linux__)
 return "MC_OS_LINUX";
#else
 return "MC_OS_UNKNOWN";
#endif
}
int requestedVersion(std::string_view source) {
 const std::size_t marker = source.find("#version");
 if(marker == std::string_view::npos) return 0;
 const std::size_t end = source.find('\n', marker);
 std::istringstream directive(std::string(source.substr(marker + 8, end == std::string_view::npos ? end : end - marker - 8)));
 int requested = 0;
 directive >> requested;
 return requested;
}
} // namespace
void captureGlShaderSnapshot() {
 std::lock_guard lock(gSnapshotMutex);
 if(gSnapshot.captured || !hasGlContext()) return;
 const auto version = [](unsigned int name) {
  const char* text = reinterpret_cast<const char*>(::glGetString(name));
  int major = 0;
  int minor = 0;
  if(text != nullptr) std::sscanf(text, "%d.%d", &major, &minor);
  return major > 0 ? major * 100 + (minor >= 10 ? minor : minor * 10) : 330;
 };
 gSnapshot.glVersion = version(0x1F02);
 gSnapshot.glslVersion = version(0x8B8C);
 int buffers = 0;
 ::glGetIntegerv(0x8CDF, &buffers);
 gSnapshot.colorBuffers = std::clamp(buffers, 1, render::kMaxColorAttachments);
 captureDriverMacros(gSnapshot.vendorMacro, gSnapshot.rendererMacro);
 gSnapshot.extensions = supportedGlExtensions();
 gSnapshot.captured = true;
}
int glVersionMacro() {
 captureGlShaderSnapshot();
 std::lock_guard lock(gSnapshotMutex);
 return gSnapshot.glVersion;
}
int glslVersionMacro() {
 captureGlShaderSnapshot();
 std::lock_guard lock(gSnapshotMutex);
 return gSnapshot.glslVersion;
}
int maxColorBuffers() {
 captureGlShaderSnapshot();
 std::lock_guard lock(gSnapshotMutex);
 return gSnapshot.colorBuffers;
}
std::string vendorMacroName() {
 captureGlShaderSnapshot();
 std::lock_guard lock(gSnapshotMutex);
 return gSnapshot.vendorMacro;
}
std::string rendererMacroName() {
 captureGlShaderSnapshot();
 std::lock_guard lock(gSnapshotMutex);
 return gSnapshot.rendererMacro;
}
std::vector<std::string> glShaderExtensions() {
 captureGlShaderSnapshot();
 std::lock_guard lock(gSnapshotMutex);
 return gSnapshot.extensions;
}
std::string formatVersion122(std::string_view semver) {
 std::size_t index = 0;
 while(index < semver.size() && (semver[index] < '0' || semver[index] > '9')) ++index;
 if(index >= semver.size()) return {};
 const auto readDigits = [&](std::string& out) {
  out.clear();
  while(index < semver.size() && semver[index] >= '0' && semver[index] <= '9') out.push_back(semver[index++]);
 };
 std::string major;
 readDigits(major);
 if(major.empty() || index >= semver.size() || semver[index] != '.') return {};
 ++index;
 std::string minor;
 readDigits(minor);
 if(minor.empty()) return {};
 std::string patch = "0";
 if(index < semver.size() && semver[index] == '.') {
  ++index;
  std::string value;
  readDigits(value);
  if(!value.empty()) patch = std::move(value);
 }
 if(minor.size() == 1) minor.insert(minor.begin(), '0');
 if(patch.size() == 1) patch.insert(patch.begin(), '0');
 return major + minor + patch;
}
std::string versionPreamble(const PackDefinition& pack, const std::string& source, bool compute) {
 return versionPreambleForStages(pack, {source}, compute ? 430 : 330);
}
std::vector<ShaderMacro> engineMacros(const PackDefinition& pack) {
 std::vector<ShaderMacro> macros;
 macros.push_back({"MC_VERSION", formatVersion122("1.7.3")});
 macros.push_back({"MC_GL_VERSION", std::to_string(glVersionMacro())});
 macros.push_back({"MC_GLSL_VERSION", std::to_string(glslVersionMacro())});
 macros.push_back({"IS_IRIS", {}});
 macros.push_back({"IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS", {}});
 macros.push_back({"IRIS_VERSION", formatVersion122("1.9.2")});
 macros.push_back({"MAX_COLOR_BUFFERS", std::to_string(maxColorBuffers())});
 macros.push_back({"IRIS_HAS_TRANSLUCENCY_SORTING", {}});
 macros.push_back({"IRIS_TAG_SUPPORT", "2"});
 macros.push_back({std::string(hostOsMacro()), {}});
 macros.push_back({"MC_HAND_DEPTH", "0.125"});
 macros.push_back({"MC_MIPMAP_LEVEL", std::to_string(std::max(0, pack.mcMipmapLevel))});
 macros.push_back({vendorMacroName(), {}});
 macros.push_back({rendererMacroName(), {}});
 macros.push_back({"MC_NORMAL_MAP", {}});
 macros.push_back({"MC_SPECULAR_MAP", {}});
 macros.push_back({"MC_RENDER_QUALITY", "1.0"});
 macros.push_back({"MC_SHADOW_QUALITY", "1.0"});
 if(pack.labPbr || pack.labPbr13) macros.push_back({"MC_TEXTURE_FORMAT_LAB_PBR", {}});
 if(pack.labPbr13) macros.push_back({"MC_TEXTURE_FORMAT_LAB_PBR_1_3", {}});
 constexpr std::array<std::string_view, 3> precipitation = {"NONE", "RAIN", "SNOW"};
 constexpr std::array<std::string_view, 16> dhBlocks = {
     "UNKNOWN", "LEAVES", "STONE", "WOOD", "METAL", "DIRT", "LAVA", "DEEPSLATE", "SNOW", "SAND",
     "TERRACOTTA", "NETHER_STONE", "WATER", "GRASS", "AIR", "ILLUMINATED"};
 appendIndexedMacros(macros, "PPT_", precipitation);
 appendIndexedMacros(macros, "CAT_", kBiomeCategoryNames);
 appendIndexedMacros(macros, "BIOME_", kBiomeNames);
 appendIndexedMacros(macros, "DH_BLOCK_", dhBlocks);
 constexpr std::array<std::string_view, 24> renderStages = {
     "NONE", "SKY", "SUNSET", "CUSTOM_SKY", "SUN", "MOON", "STARS", "VOID", "TERRAIN_SOLID",
     "TERRAIN_CUTOUT_MIPPED", "TERRAIN_CUTOUT", "ENTITIES", "BLOCK_ENTITIES", "DESTROY", "OUTLINE",
     "DEBUG", "HAND_SOLID", "TERRAIN_TRANSLUCENT", "TRIPWIRE", "PARTICLES", "CLOUDS", "RAIN_SNOW",
     "WORLD_BORDER", "HAND_TRANSLUCENT"};
 static_assert(static_cast<int>(core::RenderStage::HandTranslucent) + 1 == static_cast<int>(renderStages.size()));
 appendIndexedMacros(macros, "MC_RENDER_STAGE_", renderStages);
 for(const std::string_view feature : kIrisFeatureNames)
  if(featureSupported(feature)) macros.push_back({"IRIS_FEATURE_" + std::string(feature), {}});
 for(const std::string& extension : glShaderExtensions()) macros.push_back({"MC_" + extension, {}});
 return macros;
}
std::string versionPreambleForStages(const PackDefinition& pack,
                                     const std::vector<std::string_view>& sources,
                                     int minimumVersion) {
 int version = minimumVersion;
 for(const std::string_view source : sources) version = std::max(version, requestedVersion(source));
 std::string result = "#version " + std::to_string(version) + " core\n";
 for(const ShaderMacro& macro : engineMacros(pack)) {
  result += "#define " + macro.name;
  if(!macro.value.empty()) {
   result += ' ';
   result += macro.value;
  }
  result += '\n';
 }
 return result;
}
void seedEngineMacros(const PackDefinition& pack, PPMacroTable& macros) {
 for(const ShaderMacro& macro : engineMacros(pack)) {
  macros[macro.name] = PPMacro{false, {}, macro.value.empty() ? std::string("1") : macro.value};
 }
 for(const std::string& extension : glShaderExtensions()) {
  macros[extension] = PPMacro{false, {}, "1"};
 }
}
std::string normalizePackSource(const PackDefinition& pack, const std::string& source) {
 if(source.find('#') == std::string::npos) return source;
 const auto snapshotCaptured = [] {
  captureGlShaderSnapshot();
  std::lock_guard lock(gSnapshotMutex);
  return gSnapshot.captured;
 };
 const auto engineSeed = [&]() -> const PPMacroTable& {
  static const PPMacroTable offline = [] {
   PPMacroTable macros;
   seedEngineMacros(PackDefinition{}, macros);
   return macros;
  }();
  if(!snapshotCaptured()) return offline;
  static const PPMacroTable live = [] {
   PPMacroTable macros;
   seedEngineMacros(PackDefinition{}, macros);
   return macros;
  }();
  return live;
 };
 PPMacroOverlay macros(engineSeed());
 ppAssign(macros, "MC_MIPMAP_LEVEL",
          PPMacro{false, {}, std::to_string(std::max(0, pack.mcMipmapLevel))});
 if(pack.labPbr || pack.labPbr13) ppAssign(macros, "MC_TEXTURE_FORMAT_LAB_PBR", PPMacro{});
 if(pack.labPbr13) ppAssign(macros, "MC_TEXTURE_FORMAT_LAB_PBR_1_3", PPMacro{});
 for(const std::string& feature : pack.requiredFeatures)
  if(featureSupported(feature)) ppAssign(macros, "IRIS_FEATURE_" + feature, PPMacro{});
 for(const std::string& feature : pack.optionalFeatures)
  if(featureSupported(feature)) ppAssign(macros, "IRIS_FEATURE_" + feature, PPMacro{});
 ConditionalState conditionals(ConditionalState::Flavor::Glsl);
 std::string extensions;
 std::string body;
 body.reserve(source.size());
 std::string_view srcView(source);
 std::size_t lineStart = 0;
 auto getNextLine = [&](std::string_view& outLine) -> bool {
  if(lineStart >= srcView.size()) return false;
  std::size_t lineEnd = srcView.find('\n', lineStart);
  if(lineEnd != std::string_view::npos) {
   outLine = srcView.substr(lineStart, lineEnd - lineStart);
   lineStart = lineEnd + 1;
  } else {
   outLine = srcView.substr(lineStart);
   lineStart = srcView.size();
  }
  if(!outLine.empty() && outLine.back() == '\r') outLine.remove_suffix(1);
  return true;
 };
 std::string_view physicalView;
 bool inBlockComment = false;
 while(getNextLine(physicalView)) {
  std::string logical(physicalView);
  int continuations = 0;
  while(!logical.empty() && logical.back() == '\\') {
   logical.pop_back();
   std::string_view nextView;
   if(!getNextLine(nextView)) break;
   logical += nextView;
   ++continuations;
  }
  const auto emit = [&](std::string_view text = {}) {
   body += text;
   body.append(static_cast<std::size_t>(continuations + 1), '\n');
  };
  std::string parsedLine;
  parsedLine.reserve(logical.size());
  for(std::size_t i = 0; i < logical.size();) {
   if(inBlockComment) {
    const std::size_t close = logical.find("*/", i);
    if(close == std::string::npos) break;
    i = close + 2;
    inBlockComment = false;
    parsedLine.push_back(' ');
    continue;
   }
   if(logical.compare(i, 2, "//") == 0) break;
   if(logical.compare(i, 2, "/*") == 0) {
    inBlockComment = true;
    i += 2;
    continue;
   }
   parsedLine.push_back(logical[i++]);
  }
  const std::size_t first = parsedLine.find_first_not_of(" \t\r\n");
  const std::string cleaned = first == std::string::npos ? std::string{} : parsedLine.substr(first);
  std::string keyword;
  std::string rest;
  if(parseDirective(cleaned, keyword, rest)) {
   if(keyword == "if") {
    conditionals.push(evaluateIfExpression(rest, macros));
    emit();
    continue;
   }
   if(keyword == "ifdef") {
    conditionals.push(ppContains(macros, trimmedView(rest)));
    emit();
    continue;
   }
   if(keyword == "ifndef") {
    conditionals.push(!ppContains(macros, trimmedView(rest)));
    emit();
    continue;
   }
   if(keyword == "elif") {
    conditionals.elif(evaluateIfExpression(rest, macros));
    emit();
    continue;
   }
   if(keyword == "else") {
    conditionals.else_();
    emit();
    continue;
   }
   if(keyword == "endif") {
    conditionals.endif();
    emit();
    continue;
   }
   if(keyword == "define") {
    if(conditionals.active()) {
     parseDefineDirective(rest, macros);
     emit(logical);
    } else {
     emit();
    }
    continue;
   }
   if(keyword == "undef") {
    if(conditionals.active()) {
     ppErase(macros, trimmedView(rest));
     emit(logical);
    } else {
     emit();
    }
    continue;
   }
   if(keyword == "version") {
    emit();
    continue;
   }
   if(keyword == "extension") {
    if(conditionals.active()) extensions += "#extension " + rest + '\n';
    emit();
    continue;
   }
   if(keyword == "include" || keyword == "warning" || keyword == "custom" || keyword == "moj_import") {
    emit();
    continue;
   }
  }
  if(conditionals.active())
   emit(logical);
  else
   emit();
 }
 return extensions + body;
}
std::string defaultCompositeVertexShader() {
 return GlslSnippets::get("default_composite.vsh");
}
std::string defaultRasterVertexShader() {
 return GlslSnippets::get("default_raster.vsh");
}
std::string prepareSource(const std::string& programName,
                          ShaderStage stage,
                          const PackDefinition& pack,
                          const std::string& source,
                          ShaderTransformContext context) {
 std::string prepared = normalizePackSource(pack, source);
 prepared = canonicalizeCoreSource(programName, stage, pack, std::move(prepared), context);
 return mergeColorWheelMaterial(programName, stage, std::move(prepared));
}
} // namespace net::minecraft::client::render
