#include "net/minecraft/client/render/shaderpack/WorldProgramBinder.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackInstance.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackResources.hpp"
namespace net::minecraft::client::render::shaderpack {
namespace {
constexpr unsigned int kTexture2D = 0x0DE1;
}
void bindWorldProgram(gl::ShaderProgram& program, const WorldProgramBindContext& context) {
 program.applyDrawBuffers(context.pack != nullptr ? context.pack->colorTargets.colorCount() : 1);
 if(context.uniforms != nullptr) {
  uploadShaderUniforms(program, *context.uniforms, true);
  if(context.pack != nullptr) {
   context.pack->customUniforms.upload(program);
  }
 }
 int unit = 1;
 const int maxUnits = glutil::maxTextureUnits();
 if(context.lightmapTexture != nullptr && *context.lightmapTexture != 0 && unit < maxUnits &&
    program.location("lightmap") >= 0) {
  core::activeTexture(gl::tex::Texture0 + unit);
  core::bindTexture(static_cast<int>(*context.lightmapTexture));
  program.set1i("lightmap", unit++);
 }
 const auto bindOptional = [&](const char* name, unsigned int texture) {
  if(texture == 0 || unit >= maxUnits || program.location(name) < 0) {
   return;
  }
  core::activeTexture(gl::tex::Texture0 + unit);
  core::bindTexture(static_cast<int>(texture));
  program.set1i(name, unit++);
 };
 if(context.bindTextureAtlases) {
  const int atlasSize[2] = {context.atlasWidth, context.atlasHeight};
  program.set2iAt(program.location("atlasSize"), atlasSize);
  bindOptional("normals", context.normalTexture);
  bindOptional("specular", context.specularTexture);
 }
 if(context.noiseTexture != 0 && unit < maxUnits && program.location("noisetex") >= 0) {
  core::activeTexture(gl::tex::Texture0 + unit);
  core::bindTexture(static_cast<int>(context.noiseTexture));
  program.set1i("noisetex", unit++);
 }
 if(context.pack != nullptr) {
  std::unordered_map<std::string, int> customTextures;
  std::unordered_map<std::string, int> volumes;
  ShaderPackResources::addTextures(*context.pack, "gbuffers", customTextures, volumes);
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
    gl::GLCore::bindSampler(static_cast<unsigned int>(unit), glutil::samplerObject(compare));
   }
   program.set1i(name, unit++);
  };
  const bool shadowtex0Compare =
      hw0 && program.samplerKind("shadowtex0") == gl::ShaderProgram::SamplerKind::Shadow;
  const bool shadowtex1Compare =
      hw1 && program.samplerKind("shadowtex1") == gl::ShaderProgram::SamplerKind::Shadow;
  bindShadow("shadowtex0", context.shadowDepthTexture, shadowtex0Compare);
  const int opaqueDepth =
      context.shadowOpaqueDepthTexture >= 0 ? context.shadowOpaqueDepthTexture : context.shadowDepthTexture;
  bindShadow("shadowtex1", opaqueDepth, shadowtex1Compare);
  bindShadow("shadowtex0HW", context.shadowDepthTexture, hw0);
  bindShadow("shadowtex1HW", opaqueDepth, hw1);
  for(int index = 0; index < context.shadowColorTextureCount; ++index) {
   if(context.shadowColorTextures == nullptr) {
    break;
   }
   bindShadow("shadowcolor" + std::to_string(index), context.shadowColorTextures[index], false);
  }
 }
 if(context.pack != nullptr) {
  ShaderPackResources::bind(*context.pack, program);
 }
 core::activeTexture(gl::tex::Texture0);
}
} // namespace net::minecraft::client::render::shaderpack
