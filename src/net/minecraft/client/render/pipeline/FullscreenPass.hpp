#pragma once
#include <array>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/debug/RenderProfiler.hpp"
namespace net::minecraft::client::render {
struct PackUniformValues;
struct PackViewportValues;
inline int shadowColorIndex(const std::string& name) {
 if(name.rfind("shadowcolor", 0) != 0) return -1;
 const std::string number = name.substr(11);
 if(number.empty() || number.find_first_not_of("0123456789") != std::string::npos) return -1;
 const int index = std::atoi(number.c_str());
 return index >= 0 && index < 8 ? index : -1;
}
inline int flipSideTextureId(int index,
                             const int* mainIds,
                             const int* altIds,
                             const PackInstance* pack,
                             const std::array<bool, 8>& flipped,
                             const PackInstance* flipPack) {
 int id = mainIds == nullptr ? -1 : mainIds[index];
 if(id >= 0 && altIds != nullptr && pack != nullptr && flipPack == pack && flipped[static_cast<std::size_t>(index)] &&
    altIds[index] >= 0) {
  id = altIds[index];
 }
 return id;
}
inline bool dispatchSetupIfNeeded(
    PackInstance& pack, const PackUniformValues& uniforms, const PackViewportValues* viewport, int width, int height,
    std::unordered_map<std::string, int>& textures,
    std::unordered_map<std::string, int>& colorImages,
    std::unordered_map<std::string, int>& volumes,
    const ColorTargets* colorTargets) {
 if(!gl::GLCore::computeSupported || (pack.setupWidth == width && pack.setupHeight == height)) {
  return true;
 }
 for(const PackInstance::RuntimePass& runtime : pack.setupPlan) {
  debug::RenderProfileScope passProfile("shader/setup", pack.definition.passes[runtime.passIndex].name);
  debug::RenderProfiler::instance().record(debug::RenderMetric::ShaderPasses);
  if(runtime.program == nullptr ||
     !ComputeDispatcher::dispatch(pack, pack.definition.passes[runtime.passIndex], *runtime.program,
                                  uniforms, viewport, textures, colorImages, volumes, colorTargets,
                                  1, 1, !pack.definition.allowConcurrentCompute)) {
   return false;
  }
 }
 if(!pack.setupPlan.empty()) {
  gl::GLCore::memoryBarrier(ComputeDispatcher::kBarrierBits);
  debug::RenderProfiler::instance().record(debug::RenderMetric::MemoryBarriers);
 }
 pack.setupWidth = width;
 pack.setupHeight = height;
 return true;
}
} // namespace net::minecraft::client::render
