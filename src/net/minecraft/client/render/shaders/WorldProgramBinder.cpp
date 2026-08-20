#include "net/minecraft/client/render/shaders/WorldProgramBinder.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Resources.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
namespace net::minecraft::client::render {
namespace {
// RenderTargets::kMaxColortex entries; shadowColorBuffers is clamped to [0,8]
// at load. Built once so a program bind does not allocate a name per sampler.
constexpr std::array<std::string_view, 32> kColortexNames = {
    "colortex0",  "colortex1",  "colortex2",  "colortex3",  "colortex4",  "colortex5",  "colortex6",  "colortex7",
    "colortex8",  "colortex9",  "colortex10", "colortex11", "colortex12", "colortex13", "colortex14", "colortex15",
    "colortex16", "colortex17", "colortex18", "colortex19", "colortex20", "colortex21", "colortex22", "colortex23",
    "colortex24", "colortex25", "colortex26", "colortex27", "colortex28", "colortex29", "colortex30", "colortex31"};
constexpr std::array<std::string_view, 8> kShadowColorNames = {
    "shadowcolor0", "shadowcolor1", "shadowcolor2", "shadowcolor3",
    "shadowcolor4", "shadowcolor5", "shadowcolor6", "shadowcolor7"};
bool customSampler(const PackInstance* pack, std::string_view name) {
 if(pack == nullptr) return false;
 // Probed once per candidate sampler name per program bind. find() on these
 // std::string-keyed maps would materialise a std::string per probe; both maps
 // hold only the pack's custom textures, so scanning them is allocation-free
 // and skips entirely for the common empty case.
 for(const auto& [key, texture] : pack->worldTextures) {
  if(texture > 0 && key == name) return true;
 }
 for(const auto& [volume, texture] : pack->worldVolumeTextures) {
  if(texture > 0 && volume == name) return true;
 }
 return false;
}
bool bindSceneSampler(gl::ShaderProgram& program,
                      int texture,
                      int unit,
                      const PackInstance* pack,
                      std::initializer_list<std::string_view> names) {
 if(texture <= 0) return false;
 auto first = names.end();
 int firstLocation = -1;
 for(auto it = names.begin(); it != names.end(); ++it) {
  if(customSampler(pack, *it)) continue;
  const int location = program.location(*it);
  if(location >= 0) {
   first = it;
   firstLocation = location;
   break;
  }
 }
 if(first == names.end()) return false;
 core::activeTexture(gl::tex::Texture0 + unit);
 core::bindTexture(texture);
 if(gl::GLCore::bindSampler != nullptr) gl::GLCore::bindSampler(static_cast<unsigned int>(unit), 0);
 program.set1iAt(firstLocation, unit);
 for(auto it = first + 1; it != names.end(); ++it) {
  if(customSampler(pack, *it)) continue;
  program.set1iAt(program.location(*it), unit);
 }
 return true;
}
}
void bindProgramMaterialTextures(gl::ShaderProgram& program, const WorldProgramBindContext& context) {
 if(!context.bindTextureAtlases) {
  return;
 }
 const int maxUnits = maxTextureUnits();
 const int atlasSize[2] = {context.atlasWidth, context.atlasHeight};
 program.set2iAt(program.location("atlasSize"), atlasSize);
 int pbrTextureFlags = context.pbrTextureFlags;
 if(maxUnits <= 2) pbrTextureFlags &= ~1;
 if(maxUnits <= 3) pbrTextureFlags &= ~2;
 program.set1i("pbrTextureFlags", pbrTextureFlags);
 program.set1i("pbrAtlasGrid", std::max(1, context.pbrAtlasGrid));
 if(context.normalTexture != 0 && maxUnits > 2) {
  bindSceneSampler(program, static_cast<int>(context.normalTexture), 2, context.pack,
                   {"normals", "gtexture1", "normalMap"});
 }
 if(context.specularTexture != 0 && maxUnits > 3) {
  bindSceneSampler(program, static_cast<int>(context.specularTexture), 3, context.pack,
                   {"specular", "gtexture2", "specularMap"});
 }
 core::activeTexture(gl::tex::Texture0);
}
void bindWorldProgram(gl::ShaderProgram& program, const WorldProgramBindContext& context) {
 if(context.uniforms != nullptr) {
  uploadShaderUniforms(program, *context.uniforms, true);
  if(context.pack != nullptr) {
   context.pack->customUniforms.upload(program);
  }
 }
 const int maxUnits = maxTextureUnits();
 if(context.lightmapTexture != 0 && maxUnits > 1) {
  const int location = program.location("lightmap");
  if(location >= 0) {
   core::activeTexture(gl::tex::Texture0 + 1);
   core::bindTexture(static_cast<int>(context.lightmapTexture));
   program.set1iAt(location, 1);
  }
 }
 int unit = 4;
 if(context.overlayTexture != 0 && unit < maxUnits) {
  const int irisLocation = program.location("iris_overlay");
  const int flywheelLocation = program.location("flw_overlayTex");
  if(irisLocation >= 0 || flywheelLocation >= 0) {
   core::activeTexture(gl::tex::Texture0 + unit);
   core::bindTexture(static_cast<int>(context.overlayTexture));
   program.set1iAt(irisLocation, unit);
   program.set1iAt(flywheelLocation, unit);
   ++unit;
  }
 }
 if(context.sceneTargets != nullptr) {
  static constexpr std::array<std::string_view, 4> aliases = {"gaux1", "gaux2", "gaux3", "gaux4"};
  for(int index = ColorTargets::renderTargetSamplerStartIndex(false);
      index < context.sceneTargets->colorCount() && unit < maxUnits;
      ++index) {
   const std::string_view name = kColortexNames[static_cast<std::size_t>(index)];
   const bool bound = index < 8
                          ? bindSceneSampler(program, static_cast<int>(context.sceneTargets->readTexture(index)), unit,
                                             context.pack,
                                             {name, aliases[static_cast<std::size_t>(index - 4)]})
                          : bindSceneSampler(program, static_cast<int>(context.sceneTargets->readTexture(index)), unit,
                                             context.pack,
                                             {name});
   if(bound) ++unit;
  }
  if(unit < maxUnits &&
     bindSceneSampler(program, context.sceneDepthTexture, unit, context.pack,
                      {"depthtex0", "depthtex", "gdepthtex"}))
   ++unit;
  if(unit < maxUnits && bindSceneSampler(program, context.opaqueDepthTexture, unit, context.pack, {"depthtex1"}))
   ++unit;
  if(unit < maxUnits && bindSceneSampler(program, context.handDepthTexture, unit, context.pack, {"depthtex2"}))
   ++unit;
 }
 if(context.pack != nullptr) {
  unit = bindAvailableSamplers(program, context.pack->worldTextures,
                               context.pack->worldVolumeTextures, unit, maxUnits,
                               context.pack->definition);
 }
 if(!context.clearShadowBindsWhenNoPack) {
  const bool hw0 = context.pack != nullptr && context.pack->definition.shadowHardwareFiltering[0];
  const bool hw1 = context.pack != nullptr && context.pack->definition.shadowHardwareFiltering[1];
  const auto bindShadow = [&](std::string_view name, int texture, bool compare) {
   if(texture < 0 || unit >= maxUnits) return;
   const int location = program.location(name);
   if(location < 0 || customSampler(context.pack, name)) {
    return;
   }
   core::activeTexture(gl::tex::Texture0 + unit);
   core::bindTexture(texture);
   if(gl::GLCore::bindSampler != nullptr) {
    gl::GLCore::bindSampler(static_cast<unsigned int>(unit), samplerObject(compare));
   }
   program.set1iAt(location, unit++);
  };
  const bool separateHw =
      context.pack != nullptr &&
      (context.pack->definition.requiredFeatures.contains("SEPARATE_HARDWARE_SAMPLERS") ||
       context.pack->definition.optionalFeatures.contains("SEPARATE_HARDWARE_SAMPLERS"));
  // https://shaders.properties/current/reference/buffers/shadowtex/
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  const bool shadowtex0Compare =
      !separateHw && hw0 && program.samplerKind("shadowtex0") == gl::ShaderProgram::SamplerKind::Shadow;
  const bool shadowtex1Compare =
      !separateHw && hw1 && program.samplerKind("shadowtex1") == gl::ShaderProgram::SamplerKind::Shadow;
  bindShadow("shadowtex0", context.shadowDepthTexture, shadowtex0Compare);
  const int opaqueDepth =
      context.shadowOpaqueDepthTexture >= 0 ? context.shadowOpaqueDepthTexture : context.shadowDepthTexture;
  bindShadow("shadowtex1", opaqueDepth, shadowtex1Compare);
  if(separateHw) {
   const bool hw0Shadow =
       hw0 || program.samplerKind("shadowtex0HW") == gl::ShaderProgram::SamplerKind::Shadow;
   const bool hw1Shadow =
       hw1 || program.samplerKind("shadowtex1HW") == gl::ShaderProgram::SamplerKind::Shadow;
   bindShadow("shadowtex0HW", context.shadowDepthTexture, hw0Shadow);
   bindShadow("shadowtex1HW", opaqueDepth, hw1Shadow);
  }
  for(int index = 0; index < context.shadowColorTextureCount; ++index) {
   if(context.shadowColorTextures == nullptr) {
    break;
   }
   bindShadow(kShadowColorNames[static_cast<std::size_t>(index)], context.shadowColorTextures[index], false);
  }
 }
 if(context.pack != nullptr) {
  bindPackResources(*context.pack, program);
 }
 core::activeTexture(gl::tex::Texture0);
}
} // namespace net::minecraft::client::render
