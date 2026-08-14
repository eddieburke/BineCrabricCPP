#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Resources.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/debug/VTuneTrace.hpp"
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
 const auto groups = workGroups(pass, program.computeLocalSize(), width, height);
 VT_TRACE_COUNTER("ComputeDispatches", 1);
 VT_TRACE_COUNTER("ComputeWorkgroups", static_cast<std::uint64_t>(groups[0]) * groups[1] * groups[2]);
 const auto indirect = pack.definition.indirectDispatches.find(pass.name);
 const bool useIndirect = indirect != pack.definition.indirectDispatches.end() &&
                          gl::GLCore::dispatchComputeIndirect != nullptr &&
                          indirect->second.buffer >= 0 &&
                          indirect->second.buffer < kMaxShaderStorageBuffers &&
                          pack.bufferObjects[indirect->second.buffer].handle() != 0 &&
                          pack.bufferBytes[indirect->second.buffer] >=
                              indirect->second.offset + sizeof(unsigned int) * 3;
 if(useIndirect) {
  gl::GLCore::memoryBarrier(kCommandBarrierBit);
  VT_TRACE_COUNTER("MemoryBarriers", 1);
  gl::GLCore::bindBuffer(0x90EE, pack.bufferObjects[indirect->second.buffer].handle());
  gl::GLCore::dispatchComputeIndirect(static_cast<intptr_t>(indirect->second.offset));
  gl::GLCore::bindBuffer(0x90EE, 0);
 } else {
  gl::GLCore::dispatchCompute(groups[0], groups[1], groups[2]);
 }
 // CompositeRenderer.java renderAll: one barrier after a pass's computes, none before.
 if(barrier) {
  gl::GLCore::memoryBarrier(kBarrierBits);
  VT_TRACE_COUNTER("MemoryBarriers", 1);
 }
 return true;
}
} // namespace net::minecraft::client::render::ComputeDispatcher
