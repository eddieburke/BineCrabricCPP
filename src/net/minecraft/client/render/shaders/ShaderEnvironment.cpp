#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <string_view>
#include <utility>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/ColorWheelMerge.hpp"
#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"

namespace net::minecraft::client::render {
namespace {
using PackCatalog::lower;

struct GlShaderSnapshot {
 int glVersion = 330;
 int glslVersion = 330;
 int colorBuffers = 1;
 std::string preamble;
 std::vector<std::string> extensions;
 bool captured = false;
};

GlShaderSnapshot gSnapshot;
std::mutex gSnapshotMutex;

std::string captureDriverPreamble() {
 const auto text = [](unsigned int name) {
  const char* value = reinterpret_cast<const char*>(::glGetString(name));
  return lower(value == nullptr ? std::string{} : std::string(value));
 };
 const std::string vendor = text(0x1F00);
 const std::string renderer = text(0x1F01);
 const auto starts = [](std::string_view value, std::string_view prefix) { return value.starts_with(prefix); };
 std::string vendorMacro = "MC_GL_VENDOR_OTHER";
 if(starts(vendor, "ati")) vendorMacro = "MC_GL_VENDOR_ATI";
 else if(starts(vendor, "intel")) vendorMacro = "MC_GL_VENDOR_INTEL";
 else if(starts(vendor, "nvidia")) vendorMacro = "MC_GL_VENDOR_NVIDIA";
 else if(starts(vendor, "amd")) vendorMacro = "MC_GL_VENDOR_AMD";
 else if(starts(vendor, "x.org")) vendorMacro = "MC_GL_VENDOR_XORG";
 std::string rendererMacro = "MC_GL_RENDERER_OTHER";
 if(starts(renderer, "amd") || starts(renderer, "ati") || starts(renderer, "radeon"))
  rendererMacro = "MC_GL_RENDERER_RADEON";
 else if(starts(renderer, "gallium")) rendererMacro = "MC_GL_RENDERER_GALLIUM";
 else if(starts(renderer, "intel")) rendererMacro = "MC_GL_RENDERER_INTEL";
 else if(starts(renderer, "geforce") || starts(renderer, "nvidia")) rendererMacro = "MC_GL_RENDERER_GEFORCE";
 else if(starts(renderer, "quadro") || starts(renderer, "nvs")) rendererMacro = "MC_GL_RENDERER_QUADRO";
 else if(starts(renderer, "mesa")) rendererMacro = "MC_GL_RENDERER_MESA";
 else if(starts(renderer, "apple")) rendererMacro = "MC_GL_RENDERER_APPLE";
 return "#define " + vendorMacro + "\n#define " + rendererMacro + "\n";
}

template <std::size_t N>
void appendIndexedDefines(std::string& result,
                          std::string_view prefix,
                          const std::array<std::string_view, N>& names) {
 for(std::size_t index = 0; index < names.size(); ++index)
  result += "#define " + std::string(prefix) + std::string(names[index]) + " " + std::to_string(index) + "\n";
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
}

void captureGlShaderSnapshot() {
 std::lock_guard lock(gSnapshotMutex);
 if(gSnapshot.captured || !hasGlContext()) return;
 const auto version = [](unsigned int name) {
  const char* text = reinterpret_cast<const char*>(::glGetString(name));
  int major = 0;
  int minor = 0;
  if(text != nullptr) std::sscanf(text, "%d.%d", &major, &minor);
  return major > 0 ? major * 100 + minor * 10 : 330;
 };
 gSnapshot.glVersion = version(0x1F02);
 gSnapshot.glslVersion = version(0x8B8C);
 int buffers = 0;
 ::glGetIntegerv(0x8CDF, &buffers);
 gSnapshot.colorBuffers = std::clamp(buffers, 1, render::kMaxColorAttachments);
 gSnapshot.preamble = captureDriverPreamble();
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

std::string driverPreamble() {
 captureGlShaderSnapshot();
 std::lock_guard lock(gSnapshotMutex);
 return gSnapshot.preamble;
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

std::string versionPreambleForStages(const PackDefinition& pack,
                                     const std::vector<std::string_view>& sources,
                                     int minimumVersion) {
 int version = minimumVersion;
 for(const std::string_view source : sources) version = std::max(version, requestedVersion(source));
 const std::string mcVersion = formatVersion122("1.7.3");
 const std::string irisVersion = formatVersion122("1.9.2");
 std::string result = "#version " + std::to_string(version) + " core\n";
 result += "#define MC_VERSION " + mcVersion + "\n";
 result += "#define MC_GL_VERSION " + std::to_string(glVersionMacro()) + "\n";
 result += "#define MC_GLSL_VERSION " + std::to_string(glslVersionMacro()) + "\n";
 result += "#define IS_IRIS\n";
 result += "#define IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS\n";
 result += "#define IRIS_VERSION " + irisVersion + "\n";
 result += "#define MAX_COLOR_BUFFERS " + std::to_string(maxColorBuffers()) + "\n";
 result += "#define IRIS_HAS_TRANSLUCENCY_SORTING\n";
 result += "#define IRIS_TAG_SUPPORT 2\n";
 result += "#define " + std::string(hostOsMacro()) + "\n";
 result += GlslSnippets::get("mc_hand_depth");
 result += "#define MC_MIPMAP_LEVEL " + std::to_string(std::max(0, pack.mcMipmapLevel)) + "\n";
 constexpr std::array<std::string_view, 3> precipitation = {"NONE", "RAIN", "SNOW"};
 constexpr std::array<std::string_view, 19> categories = {
     "NONE", "TAIGA", "EXTREME_HILLS", "JUNGLE", "MESA", "PLAINS", "SAVANNA", "ICY", "THE_END",
     "BEACH", "FOREST", "OCEAN", "DESERT", "RIVER", "SWAMP", "MUSHROOM", "NETHER", "MOUNTAIN", "UNDERGROUND"};
 constexpr std::array<std::string_view, 13> biomes = {
     "RAINFOREST", "SWAMP", "SEASONAL_FOREST", "FOREST", "SAVANNA", "SHRUBLAND", "TAIGA", "DESERT",
     "PLAINS", "ICE_DESERT", "TUNDRA", "NETHER_WASTES", "THE_END"};
 constexpr std::array<std::string_view, 16> dhBlocks = {
     "UNKNOWN", "LEAVES", "STONE", "WOOD", "METAL", "DIRT", "LAVA", "DEEPSLATE", "SNOW", "SAND",
     "TERRACOTTA", "NETHER_STONE", "WATER", "GRASS", "AIR", "ILLUMINATED"};
 appendIndexedDefines(result, "PPT_", precipitation);
 appendIndexedDefines(result, "CAT_", categories);
 appendIndexedDefines(result, "BIOME_", biomes);
 appendIndexedDefines(result, "DH_BLOCK_", dhBlocks);
 result += driverPreamble();
 result += "#define MC_NORMAL_MAP\n";
 result += "#define MC_SPECULAR_MAP\n";
 result += "#define MC_RENDER_QUALITY 1.0\n";
 result += "#define MC_SHADOW_QUALITY 1.0\n";
 if(pack.labPbr || pack.labPbr13) result += GlslSnippets::get("mc_texture_format_lab_pbr");
 if(pack.labPbr13) result += GlslSnippets::get("mc_texture_format_lab_pbr_13");
 constexpr std::array<std::string_view, 24> renderStages = {
     "NONE", "SKY", "SUNSET", "CUSTOM_SKY", "SUN", "MOON", "STARS", "VOID", "TERRAIN_SOLID",
     "TERRAIN_CUTOUT_MIPPED", "TERRAIN_CUTOUT", "ENTITIES", "BLOCK_ENTITIES", "DESTROY", "OUTLINE",
     "DEBUG", "HAND_SOLID", "TERRAIN_TRANSLUCENT", "TRIPWIRE", "PARTICLES", "CLOUDS", "RAIN_SNOW",
     "WORLD_BORDER", "HAND_TRANSLUCENT"};
 static_assert(static_cast<int>(core::RenderStage::HandTranslucent) + 1 == static_cast<int>(renderStages.size()));
 appendIndexedDefines(result, "MC_RENDER_STAGE_", renderStages);
 for(const std::string& feature : pack.requiredFeatures)
  if(featureSupported(feature)) result += "#define IRIS_FEATURE_" + feature + "\n";
 for(const std::string& feature : pack.optionalFeatures)
  if(featureSupported(feature)) result += "#define IRIS_FEATURE_" + feature + "\n";
 for(const std::string& extension : glShaderExtensions()) result += "#define MC_" + extension + "\n";
 if(pack.colorWheel.present) appendColorWheelMacros(result);
 return result;
}
}
