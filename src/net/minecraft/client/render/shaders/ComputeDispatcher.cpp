#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Resources.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::render::ComputeDispatcher {
bool dispatch(PackInstance& pack,
              const PackPass& pass,
              gl::ShaderProgram& program,
              const PackUniformValues& uniforms,
              const PackViewportValues* viewport,
              std::unordered_map<std::string, int>& textures,
              std::unordered_map<std::string, int>& colorImages,
              std::unordered_map<std::string, int>& volumeTextures,
              const ColorTargets* colorTargets,
              int width,
              int height,
              bool barrier) {
  program.bind();
  uploadShaderUniforms(program, uniforms, true, viewport);
  pack.customUniforms.upload(program);
  refreshTextureAliases(textures);
  bindSamplers(program, textures, volumeTextures, maxTextureUnits(), pack.definition);
  const unsigned int nextImageUnit = bindColorImages(program, colorImages, pack.definition, colorTargets);
   bindPackResources(pack, program, nextImageUnit);
  const auto groups = workGroups(pass, width, height);
   const auto indirect = pack.definition.indirectDispatches.find(pass.name);
   const bool useIndirect = indirect != pack.definition.indirectDispatches.end() &&
                            gl::GLCore::dispatchComputeIndirect != nullptr &&
                            indirect->second.buffer >= 0 &&
                            indirect->second.buffer < kMaxShaderStorageBuffers &&
                             pack.bufferObjects[indirect->second.buffer].handle() != 0 &&
                            pack.bufferBytes[indirect->second.buffer] >=
                                indirect->second.offset + sizeof(unsigned int) * 3;
  for(int iteration = 0; iteration < pass.iterations; ++iteration) {
  if(barrier || useIndirect) {
   unsigned int bits = barrier ? kBarrierBits : 0u;
   if(useIndirect) bits |= kCommandBarrierBit;
   gl::GLCore::memoryBarrier(bits);
  }
  if(useIndirect) {
    gl::GLCore::bindBuffer(0x90EE, pack.bufferObjects[indirect->second.buffer].handle());
   gl::GLCore::dispatchComputeIndirect(static_cast<intptr_t>(indirect->second.offset));
   gl::GLCore::bindBuffer(0x90EE, 0);
  } else {
   gl::GLCore::dispatchCompute(groups[0], groups[1], groups[2]);
  }
 }
 // The barrier above only orders one iteration against the next. Whoever samples
 // what this pass wrote — the next compute parent, or the raster pass that reads
 // the voxel/floodfill targets — runs after the loop exits, so the last dispatch
 // needs its own barrier.
 if(barrier) {
  gl::GLCore::memoryBarrier(kBarrierBits);
 }
 return true;
}
} // namespace net::minecraft::client::render::ComputeDispatcher
