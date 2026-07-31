#include "net/minecraft/client/render/shaderpack/ShaderPackInstance.hpp"
#include <algorithm>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
namespace net::minecraft::client::render::shaderpack {
void ShaderPackInstance::clearGpuResources() {
 colorTargets.destroy();
 publishedTextures.clear();
 for(auto& [name, image] : images) {
  if(image.texture != 0) {
   core::deleteTexture(image.texture);
  }
 }
 images.clear();
 if(noiseTexture != 0) {
  core::deleteTexture(noiseTexture);
 }
 noiseTexture = 0;
 noiseResolution = 0;
 for(int i = 0; i < 2; ++i) {
  if(depthTextures[i] != 0) {
   core::deleteTexture(depthTextures[i]);
  }
  depthTextures[i] = 0;
  depthTextureW[i] = 0;
  depthTextureH[i] = 0;
 }
 for(auto& [name, texture] : customTextures) {
  if(texture != 0 && ownedCustomTextures.contains(texture)) {
   core::deleteTexture(texture);
  }
 }
 customTextures.clear();
 ownedCustomTextures.clear();
 if(gl::GLCore::deleteBuffers != nullptr) {
  for(unsigned int& buffer : bufferObjects) {
   if(buffer != 0) {
    gl::GLCore::deleteBuffers(1, &buffer);
    buffer = 0;
   }
  }
 }
 std::fill(std::begin(bufferBytes), std::end(bufferBytes), 0);
 setupWidth = 0;
 setupHeight = 0;
}
ShaderPackInstance::~ShaderPackInstance() {
 clearGpuResources();
}
void ShaderPackInstance::applyPassBuckets(render::PackPassBuckets&& buckets) {
 postPasses = std::move(buckets.postPasses);
 deferredPasses = std::move(buckets.deferredPasses);
 computePasses = std::move(buckets.computePasses);
 beginPasses = std::move(buckets.beginPasses);
 shadowCompositePasses = std::move(buckets.shadowCompositePasses);
 preparePasses = std::move(buckets.preparePasses);
 setupPasses = std::move(buckets.setupPasses);
}
} // namespace net::minecraft::client::render::shaderpack
