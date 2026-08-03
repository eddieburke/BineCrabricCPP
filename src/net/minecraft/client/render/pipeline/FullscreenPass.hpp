#pragma once
#include <array>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <string>
#include <unordered_map>
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::render {
struct PackUniformValues;

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

// IrisRenderingPipeline.java:640). Runs before the first fullscreen stage of a frame.
inline bool dispatchSetupIfNeeded(
    PackInstance& pack, const PackUniformValues& uniforms, int width, int height,
    std::unordered_map<std::string, int>& textures,
    std::unordered_map<std::string, int>& colorImages,
    std::unordered_map<std::string, int>& volumes,
    const ColorTargets* colorTargets,
    const std::function<gl::ShaderProgram*(PackInstance&, const std::string&)>& compileFn) {
 if(!gl::GLCore::computeSupported || (pack.setupWidth == width && pack.setupHeight == height)) {
  return true;
 }
 for(std::size_t passIndex : pack.setupPasses) {
  if(!ComputeDispatcher::dispatch(pack, pack.definition.passes[passIndex], uniforms, textures, colorImages,
                                  volumes, colorTargets, width, height, !pack.definition.allowConcurrentCompute,
                                  compileFn)) {
   return false;
  }
 }
 if(pack.definition.allowConcurrentCompute && !pack.setupPasses.empty()) {
  gl::GLCore::memoryBarrier(ComputeDispatcher::kBarrierBits);
 }
 pack.setupWidth = width;
 pack.setupHeight = height;
 return true;
}

} // namespace net::minecraft::client::render
