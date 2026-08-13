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
bool customSampler(const PackInstance* pack, std::string_view name) {
 if(pack == nullptr) return false;
 const auto texture = pack->worldTextures.find(std::string(name));
 if(texture != pack->worldTextures.end() && texture->second > 0) return true;
 const auto volume = pack->worldVolumeTextures.find(std::string(name));
 return volume != pack->worldVolumeTextures.end() && volume->second > 0;
}
bool bindSceneSampler(gl::ShaderProgram& program,
                      int texture,
                      int unit,
                      const PackInstance* pack,
                      std::initializer_list<std::string_view> names) {
 if(texture <= 0) return false;
 bool declared = false;
 for(const std::string_view name : names) {
  declared = declared || (program.location(name) >= 0 && !customSampler(pack, name));
 }
 if(!declared) return false;
 core::activeTexture(gl::tex::Texture0 + unit);
 core::bindTexture(texture);
 if(gl::GLCore::bindSampler != nullptr) gl::GLCore::bindSampler(static_cast<unsigned int>(unit), 0);
 for(const std::string_view name : names) {
  if(program.location(name) >= 0 && !customSampler(pack, name)) program.set1i(name, unit);
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
 if(context.lightmapTexture != 0 && maxUnits > 1 &&
    program.location("lightmap") >= 0) {
  core::activeTexture(gl::tex::Texture0 + 1);
  core::bindTexture(static_cast<int>(context.lightmapTexture));
  program.set1i("lightmap", 1);
 }
 int unit = 4;
 if(context.overlayTexture != 0 && unit < maxUnits &&
    (program.location("iris_overlay") >= 0 || program.location("flw_overlayTex") >= 0)) {
  core::activeTexture(gl::tex::Texture0 + unit);
  core::bindTexture(static_cast<int>(context.overlayTexture));
  if(program.location("iris_overlay") >= 0) program.set1i("iris_overlay", unit);
  if(program.location("flw_overlayTex") >= 0) program.set1i("flw_overlayTex", unit);
  ++unit;
 }
 if(context.sceneTargets != nullptr) {
  static constexpr std::array<std::string_view, 4> aliases = {"gaux1", "gaux2", "gaux3", "gaux4"};
  for(int index = ColorTargets::renderTargetSamplerStartIndex(false);
      index < context.sceneTargets->colorCount() && unit < maxUnits;
      ++index) {
   const std::string name = "colortex" + std::to_string(index);
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
  const auto bindShadow = [&](const std::string& name, int texture, bool compare) {
   if(texture < 0 || unit >= maxUnits || program.location(name) < 0 || customSampler(context.pack, name)) {
    return;
   }
   core::activeTexture(gl::tex::Texture0 + unit);
   core::bindTexture(texture);
   if(gl::GLCore::bindSampler != nullptr) {
    gl::GLCore::bindSampler(static_cast<unsigned int>(unit), samplerObject(compare));
   }
   program.set1i(name, unit++);
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
   bindShadow("shadowcolor" + std::to_string(index), context.shadowColorTextures[index], false);
  }
 }
 if(context.pack != nullptr) {
  bindPackResources(*context.pack, program);
 }
 core::activeTexture(gl::tex::Texture0);
}
} // namespace net::minecraft::client::render
