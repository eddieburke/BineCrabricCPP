#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/Pack.hpp"
#include "net/minecraft/client/render/Catalog.hpp"
#include "net/minecraft/client/render/SourceProcessor.hpp"
#include "net/minecraft/client/render/ShaderFail.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderTargets.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace net::minecraft::client::render::glutil {
using pack_catalog::lower;

unsigned int samplerObject(bool compare, bool linear, bool mipmap) {
 if(!compare || !gl::GLCore::samplerObjectsSupported) return 0;
 static unsigned int samplers[4]{};
 const int index = (linear ? 1 : 0) | (mipmap ? 2 : 0);
 if(samplers[index] == 0) {
  gl::GLCore::genSamplers(1, &samplers[index]);
  const int mag = linear ? gl::filter::Linear : gl::filter::Nearest;
  const int min =
      mipmap ? (linear ? 0x2703 : 0x2700) : (linear ? gl::filter::Linear : gl::filter::Nearest);
  gl::GLCore::samplerParameteri(samplers[index], gl::tex::MagFilter, mag);
  gl::GLCore::samplerParameteri(samplers[index], gl::tex::MinFilter, min);
  gl::GLCore::samplerParameteri(samplers[index], gl::tex::WrapS, gl::wrap::ClampToEdge);
  gl::GLCore::samplerParameteri(samplers[index], gl::tex::WrapT, gl::wrap::ClampToEdge);
  gl::GLCore::samplerParameteri(samplers[index], 0x884C, 0x884E);
  gl::GLCore::samplerParameteri(samplers[index], 0x884D, gl::compare::Lequal);
 }
 return samplers[index];
}

static int g_highestSamplerUnit = -1;

namespace {
int shadowBufferIndex(std::string_view name, const PackDefinition* definition) {
 if(name == "shadowtex1" || name == "shadowtex1HW") return 1;
 if(name == "shadow" && definition != nullptr && definition->usesWaterShadow) return 1;
 return 0;
}
bool isShadowDepthSampler(std::string_view name) {
 return name == "shadowtex0" || name == "shadowtex1" || name == "shadowtex0HW" || name == "shadowtex1HW" ||
        name == "shadow" || name == "waterShadow" || name == "watershadow";
}
bool isShadowColorSampler(std::string_view name) {
 return name == "shadowcolor" || name.rfind("shadowcolor", 0) == 0;
}
bool isShadowRelatedSampler(std::string_view name) {
 return isShadowDepthSampler(name) || isShadowColorSampler(name);
}
void bindOneSamplerUnit(gl::ShaderProgram& program, const std::string& name, unsigned int tex, bool volume,
                        int unit, const PackDefinition* definition) {
 core::activeTexture(gl::tex::Texture0 + unit);
 if(volume) {
  ::glBindTexture(kTexture3D, tex);
 } else {
  core::bindTexture(kTexture2D, static_cast<int>(tex));
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
}

void shadowSampleMode(std::string_view name, bool sampler2DShadow, const PackDefinition* definition,
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
 if(definition != nullptr) {
  linear = !definition->shadowtexNearest[idx];
  mipmap = definition->shadowtexMipmap[idx];
 }
 const bool hwName = name.ends_with("HW");
 const bool separateHw =
     definition != nullptr && featureEnabled(*definition, "SEPARATE_HARDWARE_SAMPLERS");
 const bool hwFilter = definition != nullptr && definition->shadowHardwareFiltering[idx];
 if(hwName) {
  // RenderPearl requires SEPARATE_HARDWARE_SAMPLERS and samples *HW as sampler2DShadow;
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
                  const PackDefinition* definition) {
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

void putShadowTextures(std::unordered_map<std::string, int>& textures,
                       int shadowtex0,
                       int shadowtex1,
                       const int* shadowColorTextures,
                       int shadowColorCount,
                       const PackDefinition* definition) {
 // https://shaders.properties/current/reference/buffers/shadowtex/
 // https://shaders.properties/current/reference/buffers/shadowcolor/
 if(shadowtex0 >= 0) {
  textures["shadowtex0"] = shadowtex0;
  const int opaque = shadowtex1 >= 0 ? shadowtex1 : shadowtex0;
  textures["shadowtex1"] = opaque;
  if(definition != nullptr && featureEnabled(*definition, "SEPARATE_HARDWARE_SAMPLERS")) {
   textures["shadowtex0HW"] = shadowtex0;
   textures["shadowtex1HW"] = opaque;
  }
 }
 for(int i = 0; i < std::min(shadowColorCount, 8); ++i) {
  if(shadowColorTextures != nullptr && shadowColorTextures[i] >= 0) {
   textures["shadowcolor" + std::to_string(i)] = shadowColorTextures[i];
  }
 }
 refreshTextureAliases(textures, definition != nullptr && definition->usesWaterShadow);
}

int bindShadowSamplers(gl::ShaderProgram& program,
                       int startUnit,
                       int maxUnits,
                       const std::unordered_map<std::string, int>& textures,
                       const PackDefinition* definition) {
 int unit = startUnit;
 for(const std::string& name : program.declaredSamplers()) {
  if(!isShadowRelatedSampler(name)) continue;
  if(unit >= maxUnits) break;
  if(program.location(name) < 0) continue;
  const auto it = textures.find(name);
  if(it == textures.end() || it->second <= 0) continue;
  bindOneSamplerUnit(program, name, static_cast<unsigned int>(it->second), false, unit, definition);
  ++unit;
 }
 return unit;
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
     std::pair{"gcolor", "colortex0"},     std::pair{"gdepth", "colortex1"},
     std::pair{"gnormal", "colortex2"},   std::pair{"composite", "colortex3"},
     std::pair{"gaux1", "colortex4"},     std::pair{"gaux2", "colortex5"},
     std::pair{"gaux3", "colortex6"},     std::pair{"gaux4", "colortex7"},
     std::pair{"depthtex", "depthtex0"},  std::pair{"gdepthtex", "depthtex0"},
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

const std::vector<std::string>& supportedGlExtensions() {
 static std::vector<std::string> extensions;
 static bool initialized = false;
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

namespace {
struct FormatInfo {
 std::string_view name;
 ColorFormat format;
 unsigned int internal;
};

static constexpr std::array kFormats = {
    FormatInfo{"rgba", ColorFormat::Rgba8, 0x8058},
    FormatInfo{"rgba8", ColorFormat::Rgba8, 0x8058},
    FormatInfo{"r8", ColorFormat::R8, 0x8229},
    FormatInfo{"r16", ColorFormat::R16, 0x822A},
    FormatInfo{"r16f", ColorFormat::R16F, 0x822D},
    FormatInfo{"r32f", ColorFormat::R32F, 0x822E},
    FormatInfo{"rg8", ColorFormat::Rg8, 0x822B},
    FormatInfo{"rg16", ColorFormat::Rg16, 0x822C},
    FormatInfo{"rg16f", ColorFormat::Rg16F, 0x822F},
    FormatInfo{"rg32f", ColorFormat::Rg32F, 0x8230},
    FormatInfo{"rgb8", ColorFormat::Rgb8, 0x8051},
    FormatInfo{"rgb16", ColorFormat::Rgb16, 0x8054},
    FormatInfo{"rgb16f", ColorFormat::Rgb16F, 0x881B},
    FormatInfo{"rgb32f", ColorFormat::Rgb32F, 0x8815},
    FormatInfo{"r11f_g11f_b10f", ColorFormat::R11G11B10F, 0x8C3A},
    FormatInfo{"rgb10_a2", ColorFormat::Rgb10A2, 0x8059},
    FormatInfo{"rgb565", ColorFormat::Rgb565, 0x8D62},
    FormatInfo{"rgb5_a1", ColorFormat::Rgb5A1, 0x8057},
    FormatInfo{"rgba16", ColorFormat::Rgba16, 0x805B},
    FormatInfo{"rgba16f", ColorFormat::Rgba16F, 0x881A},
    FormatInfo{"rgba32f", ColorFormat::Rgba32F, 0x8814},
    FormatInfo{"r8ui", ColorFormat::R8Ui, 0x8232},
    FormatInfo{"r16ui", ColorFormat::R16Ui, 0x8234},
    FormatInfo{"r32ui", ColorFormat::R32Ui, 0x8236},
    FormatInfo{"rg8ui", ColorFormat::Rg8Ui, 0x8238},
    FormatInfo{"rg16ui", ColorFormat::Rg16Ui, 0x823A},
    FormatInfo{"rg32ui", ColorFormat::Rg32Ui, 0x823C},
    FormatInfo{"rgba8ui", ColorFormat::Rgba8Ui, 0x8D7C},
    FormatInfo{"rgba16ui", ColorFormat::Rgba16Ui, 0x8D76},
    FormatInfo{"rgba32ui", ColorFormat::Rgba32Ui, 0x8D70},
    FormatInfo{"r8i", ColorFormat::R8I, 0x8231},
    FormatInfo{"r16i", ColorFormat::R16I, 0x8233},
    FormatInfo{"r32i", ColorFormat::R32I, 0x8235},
    FormatInfo{"rg8i", ColorFormat::Rg8I, 0x8237},
    FormatInfo{"rg16i", ColorFormat::Rg16I, 0x8239},
    FormatInfo{"rg32i", ColorFormat::Rg32I, 0x823B},
    FormatInfo{"rgba8i", ColorFormat::Rgba8I, 0x8D8E},
    FormatInfo{"rgba16i", ColorFormat::Rgba16I, 0x8D88},
    FormatInfo{"rgba32i", ColorFormat::Rgba32I, 0x8D82}};

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
}

ColorFormat parseFormat(const std::string& format) {
 const FormatInfo* info = findFormat(format);
 if(info == nullptr) {
  shaderFatal("Unknown buffer format", "unrecognized format '" + format + "'");
 }
 return info->format;
}

const char* colorFormatName(ColorFormat format) {
 for(const FormatInfo& info : kFormats) {
  if(info.format == format) return info.name.data();
 }
 return "rgba8";
}

void texImageFormat(ColorFormat format, int& internal, unsigned& external, unsigned& type) {
 // https://shaders.properties/current/reference/constants/buffer_format/
 switch(format) {
 case ColorFormat::R8: internal = 0x8229; external = 0x1903; type = 0x1401; return;
 case ColorFormat::R16: internal = 0x822A; external = 0x1903; type = 0x1403; return;
 case ColorFormat::R16F: internal = 0x822D; external = 0x1903; type = 0x1406; return;
 case ColorFormat::R32F: internal = 0x822E; external = 0x1903; type = 0x1406; return;
 case ColorFormat::Rg8: internal = 0x822B; external = 0x8227; type = 0x1401; return;
 case ColorFormat::Rg16: internal = 0x822C; external = 0x8227; type = 0x1403; return;
 case ColorFormat::Rg16F: internal = 0x822F; external = 0x8227; type = 0x1406; return;
 case ColorFormat::Rg32F: internal = 0x8230; external = 0x8227; type = 0x1406; return;
 case ColorFormat::Rgb8: internal = 0x8051; external = 0x1907; type = 0x1401; return;
 case ColorFormat::Rgb16: internal = 0x8054; external = 0x1907; type = 0x1403; return;
 case ColorFormat::Rgb16F: internal = 0x881B; external = 0x1907; type = 0x1406; return;
 case ColorFormat::Rgb32F: internal = 0x8815; external = 0x1907; type = 0x1406; return;
 case ColorFormat::R11G11B10F: internal = 0x8C3A; external = 0x1907; type = 0x1406; return;
 case ColorFormat::Rgb10A2: internal = 0x8059; external = 0x1908; type = 0x1405; return;
 case ColorFormat::Rgb565: internal = 0x8D62; external = 0x1907; type = 0x8363; return;
 case ColorFormat::Rgb5A1: internal = 0x8057; external = 0x1908; type = 0x8034; return;
 case ColorFormat::Rgba16: internal = 0x805B; external = 0x1908; type = 0x1403; return;
 case ColorFormat::Rgba16F: internal = 0x881A; external = 0x1908; type = 0x1406; return;
 case ColorFormat::Rgba32F: internal = 0x8814; external = 0x1908; type = 0x1406; return;
 default: internal = 0x8058; external = 0x1908; type = 0x1401; return;
 }
}

unsigned int pixelFormat(std::string value) {
 static constexpr std::array<std::pair<std::string_view, unsigned int>, 11> entries = {
     std::pair<std::string_view, unsigned int>{"red", 0x1903},
     {"r", 0x1903}, {"rg", 0x8227}, {"rgb", 0x1907}, {"rgba", 0x1908},
     {"red_integer", 0x8D94}, {"redinteger", 0x8D94},
     {"rg_integer", 0x8228}, {"rginteger", 0x8228},
     {"rgba_integer", 0x8D99}, {"rgbainteger", 0x8D99}};
 return lookup(std::move(value), entries, 0x1908u);
}

unsigned int pixelType(std::string value) {
 static constexpr std::array<std::pair<std::string_view, unsigned int>, 12> entries = {
     std::pair<std::string_view, unsigned int>{"byte", 0x1400},
     {"unsigned_byte", 0x1401}, {"unsignedbyte", 0x1401},
     {"short", 0x1402}, {"unsigned_short", 0x1403}, {"unsignedshort", 0x1403},
     {"int", 0x1404}, {"unsigned_int", 0x1405}, {"unsignedint", 0x1405},
     {"float", 0x1406}, {"half_float", 0x140B}, {"halffloat", 0x140B}};
 return lookup(std::move(value), entries, 0x1401u);
}

unsigned int internalFormat(std::string value) {
 const FormatInfo* info = findFormat(std::move(value));
 return info == nullptr ? 0x8058 : info->internal;
}

unsigned int internalFormat(ColorFormat format) {
 for(const FormatInfo& info : kFormats) {
  if(info.format == format) return info.internal;
 }
 return 0x8058;
}

unsigned int bindColorImages(gl::ShaderProgram& program,
                             const std::unordered_map<std::string, int>& colorTextures,
                             const PackDefinition* definition,
                             const ColorTargets* colorTargets) {
 if(gl::GLCore::bindImageTexture == nullptr) return 0;
 unsigned int unit = 0;
 const auto bindPrefix = [&](const char* imagePrefix, const char* bufferPrefix, int count) {
  for(int index = 0; index < count && unit < 16; ++index) {
   const std::string imageName = std::string(imagePrefix) + std::to_string(index);
   if(program.location(imageName) < 0) continue;
   const std::string bufferName = std::string(bufferPrefix) + std::to_string(index);
   const auto found = colorTextures.find(bufferName);
   if(found == colorTextures.end() || found->second <= 0) continue;
   unsigned int format = 0x8058;
   if(colorTargets != nullptr) {
    format = internalFormat(colorTargets->formatOf(bufferName));
   } else if(definition != nullptr) {
    const auto target = definition->targets.find(bufferName);
    if(target != definition->targets.end()) format = internalFormat(target->second.format);
   }
   gl::GLCore::bindImageTexture(unit, static_cast<unsigned int>(found->second), 0, 0, 0, 0x88BA, format);
   program.set1i(imageName, static_cast<int>(unit));
   ++unit;
  }
 };
 bindPrefix("colorimg", "colortex", 32);
 bindPrefix("shadowcolorimg", "shadowcolor", 8);
 return unit;
}

unsigned int textureTarget(std::string value, std::size_t dimensions) {
 value = lower(std::move(value));
 if(value.find("3d") != std::string::npos || dimensions == 3) return kTexture3D;
 if(value.find("1d") != std::string::npos || dimensions == 1) return 0x0DE0;
 return kTexture2D;
}

unsigned int blendFactor(std::string value) {
 // https://shaders.properties/current/reference/shadersproperties/rendering/
 static constexpr std::array<std::pair<std::string_view, unsigned int>, 20> entries = {
     std::pair<std::string_view, unsigned int>{"zero", 0},
     {"one", 1}, {"srccolor", 0x0300}, {"src_color", 0x0300},
     {"oneminussrccolor", 0x0301}, {"one_minus_src_color", 0x0301},
     {"srcalpha", 0x0302}, {"src_alpha", 0x0302},
     {"oneminussrcalpha", 0x0303}, {"one_minus_src_alpha", 0x0303},
     {"dstalpha", 0x0304}, {"dst_alpha", 0x0304},
     {"oneminusdstalpha", 0x0305}, {"one_minus_dst_alpha", 0x0305},
     {"dstcolor", 0x0306}, {"dst_color", 0x0306},
     {"oneminusdstcolor", 0x0307}, {"one_minus_dst_color", 0x0307},
     {"src_alpha_saturate", 0x0308}, {"srcalphasaturate", 0x0308}};
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

void applyBufferBlends(const PackDefinition& pack, const std::string& program) {
 applyBufferBlends(pack, program, {});
}

void applyBufferBlends(const PackDefinition& pack, const std::string& program,
                       const std::vector<int>& rendertargets) {
 // https://shaders.properties/current/reference/shadersproperties/rendering/
 // https://github.com/IrisShaders/Iris/blob/1.20.1/src/main/java/net/irisshaders/iris/gl/blending/BlendModeStorage.java
 int drawFbo = 0;
 ::glGetIntegerv(0x8CA6, &drawFbo);
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
 for(const AlphaTestDirective& directive : pack.alphaTests) {
  if(directive.program != program) continue;
  if(!directive.enabled) {
   core::setAlphaTestRef(0.0f);
   return;
  }
  const std::string func = lower(directive.func);
  if(func == "always" || func == "gl_always") {
   core::setAlphaTestRef(0.0f);
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
 char* end = nullptr;
 const double parsed = std::strtod(input.c_str(), &end);
 if(end == input.c_str() || *end != '\0' || !std::isfinite(parsed)) {
  return false;
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

bool featureSupported(const std::string& feature) {
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

bool featureEnabled(const PackDefinition& pack, const std::string& feature) {
 return (pack.requiredFeatures.contains(feature) || pack.optionalFeatures.contains(feature)) &&
        featureSupported(feature);
}
}
