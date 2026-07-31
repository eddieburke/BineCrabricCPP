#include "net/minecraft/client/render/shaderpack/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackInstance.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackResources.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderUniforms.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::render::shaderpack::ComputeDispatcher {
bool dispatch(ShaderPackInstance& pack,
              const ShaderPass& pass,
              const ShaderUniformValues& uniforms,
              std::unordered_map<std::string, int>& textures,
              std::unordered_map<std::string, int>& colorImages,
              std::unordered_map<std::string, int>& volumeTextures,
              int width,
              int height,
              bool barrier,
              const std::function<gl::ShaderProgram*(ShaderPackInstance&, const std::string&)>& compile) {
 gl::ShaderProgram* program = compile(pack, pass.program);
 if(program == nullptr) {
  return false;
 }
 program->bind();
 uploadShaderUniforms(*program, uniforms);
 pack.customUniforms.upload(*program);
 glutil::refreshTextureAliases(textures);
 glutil::bindSamplers(*program, textures, volumeTextures, glutil::maxTextureUnits());
 const unsigned int nextImageUnit = glutil::bindColorImages(*program, colorImages, &pack.definition);
 ShaderPackResources::bind(pack, *program, nextImageUnit);
 const auto groups = workGroups(pass, width, height);
 const auto indirect = pack.definition.indirectDispatches.find(pass.name);
 const bool useIndirect = indirect != pack.definition.indirectDispatches.end() &&
                          gl::GLCore::dispatchComputeIndirect != nullptr &&
                          pack.bufferObjects[indirect->second.buffer] != 0 &&
                          pack.bufferBytes[indirect->second.buffer] >=
                              indirect->second.offset + sizeof(unsigned int) * 3;
 for(int iteration = 0; iteration < pass.iterations; ++iteration) {
  if(useIndirect) {
   gl::GLCore::bindBuffer(0x90EE, pack.bufferObjects[indirect->second.buffer]);
   gl::GLCore::dispatchComputeIndirect(static_cast<intptr_t>(indirect->second.offset));
   gl::GLCore::bindBuffer(0x90EE, 0);
  } else {
   gl::GLCore::dispatchCompute(groups[0], groups[1], groups[2]);
  }
  if(barrier) {
   gl::GLCore::memoryBarrier(kBarrierBits);
  }
 }
 return true;
}
} // namespace net::minecraft::client::render::shaderpack::ComputeDispatcher
