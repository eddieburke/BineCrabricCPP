#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include <algorithm>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
namespace net::minecraft::client::render {
void PackInstance::clearGpuResources() {
 colorTargets.destroy();
 publishedTextures.clear();
 images.clear();
 noiseTexture.reset();
 noiseResolution = 0;
 for(int i = 0; i < 2; ++i) {
  depthTextures[i].reset();
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
 std::fill(std::begin(bufferBytes), std::end(bufferBytes), 0);
 setupWidth = 0;
 setupHeight = 0;
}
PackInstance::~PackInstance() {
 clearGpuResources();
}
void PackInstance::resetPrograms() {
 compiledPrograms.clear();
 programCacheKeys.clear();
 programDrawBuffers.clear();
 logged.clear();
 programs = std::make_unique<gl::ProgramCache>();
 programState = PackProgramState::Cold;
}
bool PackInstance::rebuildRuntime(std::string& error) {
 customUniforms.setOptions(settings);
 const bool compiled = customUniforms.compile(definition.customUniforms, error);
 resetPrograms();
 programEnabledCache.clear();
 resolvedSourceCache.clear();
 PackPassBuckets buckets;
 indexPackPasses(definition, settings, buckets, programEnabledCache);
 postPasses = std::move(buckets.postPasses);
 deferredPasses = std::move(buckets.deferredPasses);
 computePasses = std::move(buckets.computePasses);
 beginPasses = std::move(buckets.beginPasses);
 shadowCompositePasses = std::move(buckets.shadowCompositePasses);
 preparePasses = std::move(buckets.preparePasses);
 setupPasses = std::move(buckets.setupPasses);
 return compiled;
}
} // namespace net::minecraft::client::render
