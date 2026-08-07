#include "net/minecraft/client/render/shaders/WorldProgramBinder.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Resources.hpp"
namespace net::minecraft::client::render {
void bindProgramMaterialTextures(gl::ShaderProgram& program, const WorldProgramBindContext& context) {
 if(!context.bindTextureAtlases) {
  return;
 }
 const int maxUnits = maxTextureUnits();
 const int atlasSize[2] = {context.atlasWidth, context.atlasHeight};
 program.set2iAt(program.location("atlasSize"), atlasSize);
 if(context.normalTexture != 0 && maxUnits > 2) {
  const int locNormals = program.location("normals");
  const int locGtex1 = program.location("gtexture1");
  const int locNormMap = program.location("normalMap");
  if(locNormals >= 0 || locGtex1 >= 0 || locNormMap >= 0) {
   core::activeTexture(gl::tex::Texture0 + 2);
   core::bindTexture(static_cast<int>(context.normalTexture));
   if(locNormals >= 0) program.set1i("normals", 2);
   if(locGtex1 >= 0) program.set1i("gtexture1", 2);
   if(locNormMap >= 0) program.set1i("normalMap", 2);
  }
 }
 if(context.specularTexture != 0 && maxUnits > 3) {
  const int locSpec = program.location("specular");
  const int locGtex2 = program.location("gtexture2");
  const int locSpecMap = program.location("specularMap");
  if(locSpec >= 0 || locGtex2 >= 0 || locSpecMap >= 0) {
   core::activeTexture(gl::tex::Texture0 + 3);
   core::bindTexture(static_cast<int>(context.specularTexture));
   if(locSpec >= 0) program.set1i("specular", 3);
   if(locGtex2 >= 0) program.set1i("gtexture2", 3);
   if(locSpecMap >= 0) program.set1i("specularMap", 3);
  }
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
 if(context.overlayTexture != 0 && unit < maxUnits && program.location("iris_overlay") >= 0) {
  core::activeTexture(gl::tex::Texture0 + unit);
  core::bindTexture(static_cast<int>(context.overlayTexture));
  program.set1i("iris_overlay", unit++);
 }
 if(context.noiseTexture != 0 && unit < maxUnits && program.location("noisetex") >= 0) {
  core::activeTexture(gl::tex::Texture0 + unit);
  core::bindTexture(static_cast<int>(context.noiseTexture));
  program.set1i("noisetex", unit++);
 }
 if(context.pack != nullptr) {
  std::unordered_map<std::string, int> customTextures;
  std::unordered_map<std::string, int> volumes;
  addPackTextures(*context.pack, "gbuffers", customTextures, volumes);
  for(const auto& [name, texture] : customTextures) {
   if(texture <= 0 || unit >= maxUnits || program.location(name) < 0) continue;
   core::activeTexture(gl::tex::Texture0 + unit);
   core::bindTexture(texture);
   program.set1i(name, unit++);
  }
 }
 if(!context.clearShadowBindsWhenNoPack) {
  const bool hw0 = context.pack != nullptr && context.pack->definition.shadowHardwareFiltering[0];
  const bool hw1 = context.pack != nullptr && context.pack->definition.shadowHardwareFiltering[1];
  const auto bindShadow = [&](const std::string& name, int texture, bool compare) {
   if(texture < 0 || unit >= maxUnits || program.location(name) < 0) {
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
