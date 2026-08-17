#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/shaders/ShaderFail.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <sstream>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#endif
namespace net::minecraft::client::render {
using PackCatalog::lower;
unsigned int samplerObject(bool compare, bool linear, bool mipmap) {
 if(!compare || !gl::GLCore::samplerObjectsSupported) return 0;
 static std::array<gl::GlSampler, 4> samplers;
 const int index = (linear ? 1 : 0) | (mipmap ? 2 : 0);
 if(!samplers[index]) {
  unsigned int h = 0;
  gl::GLCore::genSamplers(1, &h);
  if(h == 0) return 0;
  samplers[index] = gl::GlSampler(h);
  const int mag = linear ? gl::filter::Linear : gl::filter::Nearest;
  const int min =
      mipmap ? (linear ? 0x2703 : 0x2700) : (linear ? gl::filter::Linear : gl::filter::Nearest);
  gl::GLCore::samplerParameteri(h, gl::tex::MagFilter, mag);
  gl::GLCore::samplerParameteri(h, gl::tex::MinFilter, min);
  gl::GLCore::samplerParameteri(h, gl::tex::WrapS, gl::wrap::ClampToEdge);
  gl::GLCore::samplerParameteri(h, gl::tex::WrapT, gl::wrap::ClampToEdge);
  gl::GLCore::samplerParameteri(h, 0x884C, 0x884E);
  gl::GLCore::samplerParameteri(h, 0x884D, gl::compare::Lequal);
 }
 return samplers[index].handle();
}
static int g_highestSamplerUnit = -1;
namespace {
int shadowBufferIndex(std::string_view name, const PackDefinition& definition) {
 if(name == "shadowtex1" || name == "shadowtex1HW") return 1;
 if(name == "shadow" && definition.usesWaterShadow) return 1;
 return 0;
}
bool isShadowDepthSampler(std::string_view name) {
 return name == "shadowtex0" || name == "shadowtex1" || name == "shadowtex0HW" || name == "shadowtex1HW" ||
        name == "shadow" || name == "waterShadow" || name == "watershadow";
}
void bindOneSamplerUnit(gl::ShaderProgram& program, const std::string& name, unsigned int tex, bool volume,
                        int unit, const PackDefinition& definition) {
 core::activeTexture(gl::tex::Texture0 + unit);
 if(volume) {
  ::glBindTexture(kTexture3D, tex);
 } else {
  core::bindTexture(gl::cap::Texture2D, static_cast<int>(tex));
 }
 bool compare = false;
 bool linear = true;
 bool mipmap = false;
 shadowSampleMode(name, program.samplerKind(name) == gl::ShaderProgram::SamplerKind::Shadow, definition,
                  compare, linear, mipmap);
 if(gl::GLCore::bindSampler != nullptr) {
  gl::GLCore::bindSampler(static_cast<unsigned int>(unit),
                          compare ? samplerObject(true, linear, mipmap) : 0);
 }
 program.set1i(name, unit);
 g_highestSamplerUnit = std::max(g_highestSamplerUnit, unit);
}
} // namespace
void shadowSampleMode(std::string_view name, bool sampler2DShadow, const PackDefinition& definition,
                      bool& compare, bool& linear, bool& mipmap) {
 // https://shaders.properties/current/reference/buffers/shadowtex/
 // https://shaders.properties/current/reference/constants/shadowhardwarefiltering/
 compare = false;
 linear = true;
 mipmap = false;
 if(!isShadowDepthSampler(name)) {
  if(sampler2DShadow) compare = true;
  return;
 }
 const int idx = shadowBufferIndex(name, definition);
 linear = !definition.shadowtexNearest[idx];
 mipmap = false; // depth mipmaps intentionally disabled (see Pipeline.cpp)
 const bool hwName = name.ends_with("HW");
 const bool separateHw = featureEnabled(definition, "SEPARATE_HARDWARE_SAMPLERS");
 const bool hwFilter = definition.shadowHardwareFiltering[idx];
 if(hwName) {
  // Some packs require SEPARATE_HARDWARE_SAMPLERS and sample *HW as sampler2DShadow;
  // enable compare when the sampler is shadow-typed even if the const directive is commented.
  // https://shaders.properties/current/reference/buffers/shadowtex/
  compare = separateHw && (hwFilter || sampler2DShadow);
  return;
 }
 if(separateHw) return;
 if(sampler2DShadow) {
  compare = true;
  return;
 }
 if(hwFilter) compare = true;
}
void bindSamplers(gl::ShaderProgram& program,
                  const std::unordered_map<std::string, int>& textures,
                  const std::unordered_map<std::string, int>& volumeTextures,
                  int maxUnits,
                  const PackDefinition& definition) {
 int unit = 0;
 for(const std::string& name : program.declaredSamplers()) {
  if(unit >= maxUnits) break;
  if(program.location(name) < 0) continue;
  const auto kind = program.samplerKind(name);
  const bool volume = kind == gl::ShaderProgram::SamplerKind::Volume;
  const auto& source = volume ? volumeTextures : textures;
  const auto it = source.find(name);
  const unsigned int tex = it != source.end() && it->second > 0 ? static_cast<unsigned int>(it->second) : 0;
  bindOneSamplerUnit(program, name, tex, volume, unit, definition);
  ++unit;
 }
}
int bindAvailableSamplers(gl::ShaderProgram& program,
                          const std::unordered_map<std::string, int>& textures,
                          const std::unordered_map<std::string, int>& volumeTextures,
                          int firstUnit,
                          int maxUnits,
                          const PackDefinition& definition) {
 int unit = firstUnit;
 for(const std::string& name : program.declaredSamplers()) {
  if(unit >= maxUnits) break;
  if(program.location(name) < 0) continue;
  const auto kind = program.samplerKind(name);
  const bool volume = kind == gl::ShaderProgram::SamplerKind::Volume;
  const auto& source = volume ? volumeTextures : textures;
  const auto found = source.find(name);
  if(found == source.end() || found->second <= 0) continue;
  bindOneSamplerUnit(program, name, static_cast<unsigned int>(found->second), volume, unit, definition);
  ++unit;
 }
 return unit;
}
void putShadowTextures(std::unordered_map<std::string, int>& textures,
                       int shadowtex0,
                       int shadowtex1,
                       const int* shadowColorTextures,
                       int shadowColorCount,
                       const PackDefinition& definition) {
 // https://shaders.properties/current/reference/buffers/shadowtex/
 // https://shaders.properties/current/reference/buffers/shadowcolor/
 if(shadowtex0 >= 0) {
  textures["shadowtex0"] = shadowtex0;
  const int opaque = shadowtex1 >= 0 ? shadowtex1 : shadowtex0;
  textures["shadowtex1"] = opaque;
  if(featureEnabled(definition, "SEPARATE_HARDWARE_SAMPLERS")) {
   textures["shadowtex0HW"] = shadowtex0;
   textures["shadowtex1HW"] = opaque;
  }
 }
 for(int i = 0; i < std::min(shadowColorCount, 8); ++i) {
  if(shadowColorTextures != nullptr && shadowColorTextures[i] >= 0) {
   textures["shadowcolor" + std::to_string(i)] = shadowColorTextures[i];
  }
 }
 refreshTextureAliases(textures, definition.usesWaterShadow);
}
void releaseSamplers(int maxUnits) {
 if(gl::GLCore::bindSampler == nullptr) return;
 const int limit = std::min(maxUnits, g_highestSamplerUnit + 1);
 for(int unit = 0; unit < limit; ++unit) {
  gl::GLCore::bindSampler(static_cast<unsigned int>(unit), 0);
 }
 g_highestSamplerUnit = -1;
}
void refreshTextureAliases(std::unordered_map<std::string, int>& textures, bool waterShadowPresent) {
 // https://shaders.properties/current/reference/buffers/shadowtex/
 static constexpr std::array aliases = {
     std::pair{"gcolor", "colortex0"}, std::pair{"gdepth", "colortex1"},
     std::pair{"gnormal", "colortex2"}, std::pair{"composite", "colortex3"},
     std::pair{"gaux1", "colortex4"}, std::pair{"gaux2", "colortex5"},
     std::pair{"gaux3", "colortex6"}, std::pair{"gaux4", "colortex7"},
     std::pair{"depthtex", "depthtex0"}, std::pair{"gdepthtex", "depthtex0"},
     std::pair{"shadowcolor", "shadowcolor0"}};
 for(const auto& [alias, canonical] : aliases) {
  const auto found = textures.find(canonical);
  if(found != textures.end()) textures[std::string(alias)] = found->second;
 }
 const auto tex0 = textures.find("shadowtex0");
 const auto tex1 = textures.find("shadowtex1");
 if(tex0 != textures.end()) {
  textures["waterShadow"] = tex0->second;
  textures["watershadow"] = tex0->second;
  textures["shadow"] = waterShadowPresent && tex1 != textures.end() ? tex1->second : tex0->second;
 }
}
std::vector<std::string> supportedGlExtensions() {
 static std::vector<std::string> extensions;
 static bool initialized = false;
 static std::mutex mutex;
 std::lock_guard lock(mutex);
 if(!initialized && hasGlContext()) {
  initialized = true;
  int count = 0;
  ::glGetIntegerv(0x821D, &count);
  if(count > 0 && gl::GLCore::getStringi != nullptr) {
   extensions.reserve(static_cast<std::size_t>(count));
   for(int i = 0; i < count; ++i) {
    const unsigned char* name = gl::GLCore::getStringi(0x1F03, static_cast<unsigned>(i));
    if(name != nullptr) {
     extensions.emplace_back(reinterpret_cast<const char*>(name));
    }
   }
  }
  if(extensions.empty()) {
   const char* names = reinterpret_cast<const char*>(::glGetString(0x1F03));
   if(names != nullptr) {
    std::istringstream stream(names);
    for(std::string name; stream >> name;) {
     extensions.push_back(std::move(name));
    }
   }
  }
  std::erase_if(extensions, [](const std::string& extension) {
   return !extension.starts_with("GL_") ||
          !std::all_of(extension.begin(), extension.end(), [](unsigned char ch) {
           return std::isalnum(ch) != 0 || ch == '_';
          });
  });
  std::sort(extensions.begin(), extensions.end());
  extensions.erase(std::unique(extensions.begin(), extensions.end()), extensions.end());
 }
 return extensions;
}
int maxTextureUnits() {
 static int units = 0;
 if(units == 0) {
  int queried = 0;
  ::glGetIntegerv(0x8872, &queried);
  units = queried > 0 ? queried : 16;
 }
 return units;
}
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/image/ImageLimits.java
unsigned int maxImageUnits() {
 static int units = 0;
 if(units == 0) {
  int queried = 0;
  ::glGetIntegerv(0x8F38, &queried);
  units = queried > 0 ? queried : 8;
 }
 return static_cast<unsigned int>(units);
}
namespace {
struct FormatInfo {
 std::string_view name;
 ColorFormat format;
};
static constexpr std::array kFormats = {
    FormatInfo{"rgba8", ColorFormat::Rgba8},
    FormatInfo{"rgba", ColorFormat::Rgba8},
    FormatInfo{"r8", ColorFormat::R8},
    FormatInfo{"r16", ColorFormat::R16},
    FormatInfo{"r16f", ColorFormat::R16F},
    FormatInfo{"r32f", ColorFormat::R32F},
    FormatInfo{"rg8", ColorFormat::Rg8},
    FormatInfo{"rg16", ColorFormat::Rg16},
    FormatInfo{"rg16f", ColorFormat::Rg16F},
    FormatInfo{"rg32f", ColorFormat::Rg32F},
    FormatInfo{"rgb8", ColorFormat::Rgb8},
    FormatInfo{"rgb16", ColorFormat::Rgb16},
    FormatInfo{"rgb16f", ColorFormat::Rgb16F},
    FormatInfo{"rgb32f", ColorFormat::Rgb32F},
    FormatInfo{"r11f_g11f_b10f", ColorFormat::R11G11B10F},
    FormatInfo{"rgb10_a2", ColorFormat::Rgb10A2},
    FormatInfo{"rgb565", ColorFormat::Rgb565},
    FormatInfo{"rgb5_a1", ColorFormat::Rgb5A1},
    FormatInfo{"rgba16", ColorFormat::Rgba16},
    FormatInfo{"rgba16f", ColorFormat::Rgba16F},
    FormatInfo{"rgba32f", ColorFormat::Rgba32F},
    FormatInfo{"r8ui", ColorFormat::R8Ui},
    FormatInfo{"r16ui", ColorFormat::R16Ui},
    FormatInfo{"r32ui", ColorFormat::R32Ui},
    FormatInfo{"rg8ui", ColorFormat::Rg8Ui},
    FormatInfo{"rg16ui", ColorFormat::Rg16Ui},
    FormatInfo{"rg32ui", ColorFormat::Rg32Ui},
    FormatInfo{"rgba8ui", ColorFormat::Rgba8Ui},
    FormatInfo{"rgba16ui", ColorFormat::Rgba16Ui},
    FormatInfo{"rgba32ui", ColorFormat::Rgba32Ui},
    FormatInfo{"r8i", ColorFormat::R8I},
    FormatInfo{"r16i", ColorFormat::R16I},
    FormatInfo{"r32i", ColorFormat::R32I},
    FormatInfo{"rg8i", ColorFormat::Rg8I},
    FormatInfo{"rg16i", ColorFormat::Rg16I},
    FormatInfo{"rg32i", ColorFormat::Rg32I},
    FormatInfo{"rgba8i", ColorFormat::Rgba8I},
    FormatInfo{"rgba16i", ColorFormat::Rgba16I},
    FormatInfo{"rgba32i", ColorFormat::Rgba32I},
    FormatInfo{"rgb8i", ColorFormat::Rgb8I},
    FormatInfo{"rgb16i", ColorFormat::Rgb16I},
    FormatInfo{"rgb32i", ColorFormat::Rgb32I},
    FormatInfo{"rgb8ui", ColorFormat::Rgb8Ui},
    FormatInfo{"rgb16ui", ColorFormat::Rgb16Ui},
    FormatInfo{"rgb32ui", ColorFormat::Rgb32Ui},
    FormatInfo{"r8_snorm", ColorFormat::R8Snorm},
    FormatInfo{"rg8_snorm", ColorFormat::Rg8Snorm},
    FormatInfo{"rgb8_snorm", ColorFormat::Rgb8Snorm},
    FormatInfo{"rgba8_snorm", ColorFormat::Rgba8Snorm},
    FormatInfo{"r16_snorm", ColorFormat::R16Snorm},
    FormatInfo{"rg16_snorm", ColorFormat::Rg16Snorm},
    FormatInfo{"rgb16_snorm", ColorFormat::Rgb16Snorm},
    FormatInfo{"rgba16_snorm", ColorFormat::Rgba16Snorm},
    FormatInfo{"rgba2", ColorFormat::Rgba2},
    FormatInfo{"rgba4", ColorFormat::Rgba4},
    FormatInfo{"r3_g3_b2", ColorFormat::R3G3B2},
    FormatInfo{"rgb10_a2ui", ColorFormat::Rgb10A2Ui},
    FormatInfo{"rgb9_e5", ColorFormat::Rgb9E5}};
const FormatInfo* findFormat(std::string value) {
 value = lower(std::move(value));
 const auto found = std::find_if(kFormats.begin(), kFormats.end(), [&](const FormatInfo& info) {
  return info.name == value;
 });
 return found == kFormats.end() ? nullptr : &*found;
}
template <typename T, std::size_t N>
T lookup(std::string value,
         const std::array<std::pair<std::string_view, T>, N>& entries,
         T fallback) {
 value = lower(std::move(value));
 const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {
  return entry.first == value;
 });
 return found == entries.end() ? fallback : found->second;
}
} // namespace
std::string_view canonicalFormatName(std::string_view format) {
 const FormatInfo* info = findFormat(std::string(format));
 return info == nullptr ? std::string_view{} : info->name;
}
ColorFormat parseFormat(const std::string& format) {
 const FormatInfo* info = findFormat(format);
 if(info == nullptr) {
  shaderFatal("Unknown buffer format", "unrecognized format '" + format + "'");
  return ColorFormat::Rgba8;
 }
 return info->format;
}
const char* colorFormatName(ColorFormat format) {
 for(const FormatInfo& info : kFormats) {
  if(info.format == format) return info.name.data();
 }
 return "rgba8";
}
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/texture/PixelFormat.java
// Java matches PixelFormat.valueOf(name.toUpperCase()), so the whole enum is legal
// and nothing else is — the no-underscore spellings this used to accept were
// invented, and the missing BGR/RGB_INTEGER rows fell through to GL_RGBA, which is
// a glTexImage error against an integer internal format rather than a wrong colour.
unsigned int pixelFormat(std::string value) {
 static constexpr std::array<std::pair<std::string_view, unsigned int>, 12> entries = {
     std::pair<std::string_view, unsigned int>{"red", 0x1903},
     {"rg", 0x8227},
     {"rgb", 0x1907},
     {"bgr", 0x80E0},
     {"rgba", 0x1908},
     {"bgra", 0x80E1},
     {"red_integer", 0x8D94},
     {"rg_integer", 0x8228},
     {"rgb_integer", 0x8D98},
     {"bgr_integer", 0x8D9A},
     {"rgba_integer", 0x8D99},
     {"bgra_integer", 0x8D9B}};
 return lookup(std::move(value), entries, 0u);
}
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/texture/PixelType.java
unsigned int pixelType(std::string value) {
 static constexpr std::array<std::pair<std::string_view, unsigned int>, 22> entries = {
     std::pair<std::string_view, unsigned int>{"byte", 0x1400},
     {"short", 0x1402},
     {"int", 0x1404},
     {"half_float", 0x140B},
     {"float", 0x1406},
     {"unsigned_byte", 0x1401},
     {"unsigned_byte_3_3_2", 0x8032},
     {"unsigned_byte_2_3_3_rev", 0x8362},
     {"unsigned_short", 0x1403},
     {"unsigned_short_5_6_5", 0x8363},
     {"unsigned_short_5_6_5_rev", 0x8364},
     {"unsigned_short_4_4_4_4", 0x8033},
     {"unsigned_short_4_4_4_4_rev", 0x8365},
     {"unsigned_short_5_5_5_1", 0x8034},
     {"unsigned_short_1_5_5_5_rev", 0x8366},
     {"unsigned_int", 0x1405},
     {"unsigned_int_8_8_8_8", 0x8035},
     {"unsigned_int_8_8_8_8_rev", 0x8367},
     {"unsigned_int_10_10_10_2", 0x8036},
     {"unsigned_int_2_10_10_10_rev", 0x8368},
     {"unsigned_int_10f_11f_11f_rev", 0x8C3B},
     {"unsigned_int_5_9_9_9_rev", 0x8C3E}};
 return lookup(std::move(value), entries, 0u);
}
// The sized GL enum comes from glFormat() in RenderTargets.hpp, which is the one
// description of a ColorFormat. kFormats carried its own copy of all 60 of them, so a
// format could be allocated with one internal enum and image-bound with another.
unsigned int internalFormat(ColorFormat format) {
 return static_cast<unsigned int>(glFormat(format).internal);
}
unsigned int internalFormat(std::string value) {
 const FormatInfo* info = findFormat(std::move(value));
 return info == nullptr ? 0x8058 : internalFormat(info->format);
}
bool integerInternalFormat(std::string value) {
 const FormatInfo* info = findFormat(std::move(value));
 return info != nullptr && isIntegerColorFormat(info->format);
}
unsigned int bindColorImages(gl::ShaderProgram& program,
                             const std::unordered_map<std::string, int>& colorTextures,
                             const PackDefinition& definition,
                             const ColorTargets* colorTargets) {
 if(gl::GLCore::bindImageTexture == nullptr) return 0;
 const unsigned int imageUnits = maxImageUnits();
 unsigned int unit = 0;
 const auto bindPrefix = [&](const char* imagePrefix, const char* bufferPrefix, int count, bool sceneColor) {
  for(int index = 0; index < count && unit < imageUnits; ++index) {
   const std::string imageName = std::string(imagePrefix) + std::to_string(index);
   if(program.location(imageName) < 0) continue;
   const std::string bufferName = std::string(bufferPrefix) + std::to_string(index);
   const auto found = colorTextures.find(bufferName);
   if(found == colorTextures.end() || found->second <= 0) continue;
   unsigned int format = 0x8058;
   if(sceneColor && colorTargets != nullptr) {
    format = internalFormat(colorTargets->formatOf(bufferName));
   } else {
    const auto target = definition.targets.find(bufferName);
    if(target != definition.targets.end()) format = internalFormat(target->second.format);
   }
   gl::GLCore::bindImageTexture(unit, static_cast<unsigned int>(found->second), 0, 0, 0, 0x88BA, format);
   program.set1i(imageName, static_cast<int>(unit));
   ++unit;
  }
 };
 bindPrefix("colorimg", "colortex", 32, true);
 bindPrefix("shadowcolorimg", "shadowcolor", 8, false);
 return unit;
}
unsigned int textureTarget(std::string value, std::size_t dimensions) {
 value = lower(std::move(value));
 if(value.find("3d") != std::string::npos || dimensions == 3) return kTexture3D;
 if(value.find("1d") != std::string::npos || dimensions == 1) return 0x0DE0;
 return gl::cap::Texture2D;
}
unsigned int blendFactor(std::string value) {
 // https://shaders.properties/current/reference/shadersproperties/rendering/
 static constexpr std::array<std::pair<std::string_view, unsigned int>, 20> entries = {
     std::pair<std::string_view, unsigned int>{"zero", 0},
     {"one", 1},
     {"srccolor", 0x0300},
     {"src_color", 0x0300},
     {"oneminussrccolor", 0x0301},
     {"one_minus_src_color", 0x0301},
     {"srcalpha", 0x0302},
     {"src_alpha", 0x0302},
     {"oneminussrcalpha", 0x0303},
     {"one_minus_src_alpha", 0x0303},
     {"dstalpha", 0x0304},
     {"dst_alpha", 0x0304},
     {"oneminusdstalpha", 0x0305},
     {"one_minus_dst_alpha", 0x0305},
     {"dstcolor", 0x0306},
     {"dst_color", 0x0306},
     {"oneminusdstcolor", 0x0307},
     {"one_minus_dst_color", 0x0307},
     {"src_alpha_saturate", 0x0308},
     {"srcalphasaturate", 0x0308}};
 return lookup(std::move(value), entries, 1u);
}
int colortexToDrawBufferIndex(const std::vector<int>& rendertargets, int colortexIndex) {
 if(colortexIndex < 0) {
  return -1;
 }
 if(rendertargets.empty()) {
  return colortexIndex;
 }
 const auto found = std::find(rendertargets.begin(), rendertargets.end(), colortexIndex);
 if(found == rendertargets.end()) {
  return -1;
 }
 return static_cast<int>(found - rendertargets.begin());
}
void applyBufferBlends(const PackDefinition& pack, const std::string& program,
                       const std::vector<int>& rendertargets) {
 // https://shaders.properties/current/reference/shadersproperties/rendering/
 // https://github.com/IrisShaders/Iris/blob/1.20.1/src/main/java/net/irisshaders/iris/gl/blending/BlendModeStorage.java
 int drawFbo = 0;
 ::glGetIntegerv(static_cast<unsigned>(gl::query::FramebufferBinding), &drawFbo);
 const bool indexed = drawFbo != 0 && gl::GLCore::perBufferBlendingSupported && gl::GLCore::blendFunci != nullptr;
 for(const BufferBlend& blend : pack.bufferBlends) {
  if(blend.program != program) continue;
  const core::BlendMode mode{static_cast<int>(blendFactor(blend.source)),
                             static_cast<int>(blendFactor(blend.destination)),
                             static_cast<int>(blendFactor(blend.sourceAlpha)),
                             static_cast<int>(blendFactor(blend.destinationAlpha))};
  if(blend.buffer < 0) {
   core::lockBlend(blend.enabled ? &mode : nullptr);
   continue;
  }
  if(!indexed) continue;
  const int drawIndex = colortexToDrawBufferIndex(rendertargets, blend.buffer);
  if(drawIndex >= 0) core::lockBufferBlend(drawIndex, blend.enabled ? &mode : nullptr);
 }
}
void applyAlphaTest(const PackDefinition& pack, const std::string& program) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ProgramDirectives.java#L80
 for(const AlphaTestDirective& directive : pack.alphaTests) {
  if(directive.program != program) continue;
  if(!directive.enabled) {
   core::setAlphaTestRef(-1.0f);
   return;
  }
  const std::string func = lower(directive.func);
  if(func == "always" || func == "gl_always") {
   core::setAlphaTestRef(-1.0f);
  } else {
   core::setAlphaTestRef(directive.ref);
  }
  return;
 }
}
bool normalizeSettingValue(const PackSetting& setting, const std::string& input, std::string& output) {
 if(setting.type == SettingType::Bool) {
  const std::string normalized = lower(input);
  if(normalized == "1" || normalized == "true" || normalized == "on") {
   output = "1";
   return true;
  }
  if(normalized == "0" || normalized == "false" || normalized == "off") {
   output = "0";
   return true;
  }
  return false;
 }
 if(setting.type == SettingType::Enum) {
  for(const std::string& candidate : setting.valueOrder) {
   if(input == candidate) {
    output = candidate;
    return true;
   }
  }
  return false;
 }
 char* end = nullptr;
 const double parsed = std::strtod(input.c_str(), &end);
 if(end == input.c_str() || *end != '\0' || !std::isfinite(parsed)) {
  return false;
 }
 for(const std::string& candidate : setting.valueOrder) {
  if(candidate == input) {
   output = candidate;
   return true;
  }
 }
 if(setting.asSlider && !setting.valueOrder.empty()) {
  double closest = std::numeric_limits<double>::max();
  for(const std::string& candidate : setting.valueOrder) {
   const double value = std::strtod(candidate.c_str(), nullptr);
   const double distance = std::abs(value - parsed);
   if(distance < closest) {
    closest = distance;
    output = candidate;
   }
  }
  return !output.empty();
 }
 if(!setting.asSlider && input == setting.defaultValue) {
  output = input;
  return true;
 }
 double value = std::clamp(parsed, setting.minimum, setting.maximum);
 value = setting.minimum + std::round((value - setting.minimum) / setting.step) * setting.step;
 value = std::clamp(value, setting.minimum, setting.maximum);
 if(setting.type == SettingType::Int) {
  output = std::to_string(static_cast<int>(std::lround(value)));
 } else {
  output = std::to_string(value);
 }
 return true;
}
bool hasGlContext() {
#ifdef _WIN32
 return wglGetCurrentContext() != nullptr;
#else
 return gl::GLCore::activeTexture != nullptr;
#endif
}
bool featureSupported(std::string_view feature) {
 // https://shaders.properties/current/reference/shadersproperties/flags/
 if(feature == "COMPUTE_SHADERS") return gl::GLCore::computeSupported;
 if(feature == "SSBO") return gl::GLCore::ssboSupported;
 if(feature == "CUSTOM_IMAGES") return gl::GLCore::bindImageTexture != nullptr;
 if(feature == "SEPARATE_HARDWARE_SAMPLERS") return gl::GLCore::samplerObjectsSupported;
 if(feature == "PER_BUFFER_BLENDING") return gl::GLCore::perBufferBlendingSupported;
 if(feature == "TESSELLATION_SHADERS" || feature == "TESSELATION_SHADERS")
  return glVersionMacro() >= 400 && gl::GLCore::patchParameteri != nullptr;
 if(feature == "ENTITY_TRANSLUCENT" || feature == "HIGHER_SHADOWCOLOR" || feature == "REVERSED_CULLING" ||
    feature == "BLOCK_EMISSION_ATTRIBUTE" || feature == "CAN_DISABLE_WEATHER" || feature == "FADE_VARIABLE" ||
    feature == "TEXTURE_FILTERING")
  return true;
 return false;
}
bool featureEnabled(const PackDefinition& pack, std::string_view feature) {
 return (pack.requiredFeatures.contains(feature) || pack.optionalFeatures.contains(feature)) &&
        featureSupported(feature);
}
} // namespace net::minecraft::client::render
