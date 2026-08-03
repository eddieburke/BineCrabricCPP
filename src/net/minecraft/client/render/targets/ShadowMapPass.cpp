#include "net/minecraft/client/render/targets/ShadowMapPass.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/render/pipeline/Manager.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::client::render::shadowmap {
namespace {
constexpr unsigned kTextureCompareMode = 0x884C;
constexpr unsigned kTextureCompareFunc = 0x884D;
constexpr int kCompareRefToTexture = 0x884E;
constexpr int kLessEqual = 0x0203;
constexpr int kNone = 0;
constexpr unsigned kTextureSwizzleRgba = 0x8B43;
constexpr int kDepthComponent32 = 0x81A7;
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
constexpr int kSwizzleR[] = {0x1903, 0x1903, 0x1903, 0x1404}; // GL_RED, GL_RED, GL_RED, GL_ONE
void applyDepthSwizzle(unsigned int texture) {
  core::bindTexture(gl::cap::Texture2D, static_cast<int>(texture));
  ::glTexParameteriv(gl::cap::Texture2D, static_cast<int>(kTextureSwizzleRgba), kSwizzleR);
}
void destroyColorPair(std::array<unsigned int, 2>& pair) {
  for(unsigned int& tex : pair) {
   if(tex != 0) {
    core::deleteTexture(tex);
    tex = 0;
   }
  }
}
} // namespace
void ShadowTargets::destroy() {
  if(shadowtex1 != 0) {
   core::deleteTexture(shadowtex1);
   shadowtex1 = 0;
  }
  shadowtex1Resolution = 0;
  for(std::array<unsigned int, 2>& pair : shadowcolor) {
   destroyColorPair(pair);
  }
  if(shadowtex0 != 0) {
   core::deleteTexture(shadowtex0);
   shadowtex0 = 0;
  }
  fbo.destroy();
  colorCount = 0;
  resolution = 0;
}
bool ShadowTargets::ensure(int resolutionIn, int colorBuffers) {
  colorBuffers = std::clamp(colorBuffers, 0, 8);
  if(valid() && resolution == resolutionIn && colorCount == colorBuffers) {
   return true;
  }
  destroy();
  gl::GLCore::ensureLoaded();
  if(!gl::GLCore::framebufferSupported || resolutionIn <= 0) {
   return false;
  }
  resolution = resolutionIn;
  colorCount = colorBuffers;
  const core::TextureBindScope textureScope;
  std::vector<int> drawBuffers;
  drawBuffers.reserve(static_cast<std::size_t>(colorBuffers));
  for(int i = 0; i < colorBuffers; ++i) {
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderTargets.java
   for(unsigned int& tex : shadowcolor[static_cast<std::size_t>(i)]) {
    tex = core::genTexture();
    core::bindTexture(gl::cap::Texture2D, static_cast<int>(tex));
    ::glTexImage2D(gl::cap::Texture2D, 0, gl::pixel::Rgba8, resolution, resolution, 0, gl::pixel::Rgba,
                   gl::pixel::UnsignedByte, nullptr);
    ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Linear);
    ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Linear);
    ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
    ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
   }
   fbo.addColorAttachment(i, shadowcolor[static_cast<std::size_t>(i)][0]);
   drawBuffers.push_back(i);
  }
  if(colorBuffers > 0) {
   if(!fbo.drawBuffers(drawBuffers)) {
    destroy();
    return false;
   }
  } else {
   // buffer (GlFramebuffer.java:57-59).
   fbo.noDrawBuffers();
   ::glReadBuffer(0);
  }
  shadowtex0 = core::genTexture();
  core::bindTexture(gl::cap::Texture2D, static_cast<int>(shadowtex0));
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderTargets.java
  ::glTexImage2D(gl::cap::Texture2D, 0, kDepthComponent32, resolution, resolution, 0, gl::pixel::DepthComponent,
                 gl::pixel::UnsignedInt, nullptr);
  const int depthFilter = depthCompare ? gl::filter::Linear : gl::filter::Nearest;
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, depthFilter);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, depthFilter);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
  ::glTexParameteri(gl::cap::Texture2D, static_cast<int>(kTextureCompareMode),
                    depthCompare ? kCompareRefToTexture : kNone);
  if(depthCompare) {
   ::glTexParameteri(gl::cap::Texture2D, static_cast<int>(kTextureCompareFunc), kLessEqual);
  }
  applyDepthSwizzle(shadowtex0);
  fbo.addDepthAttachment(shadowtex0);
  const bool ok = fbo.checkStatus() == static_cast<unsigned>(gl::framebuffer::Complete);
  gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, 0);
  if(!ok) {
   destroy();
   return false;
  }
  return true;
}
void ShadowTargets::attachCompositeColors(const std::array<bool, 8>& flipped, int colorBuffers) {
  if(!valid() || colorBuffers <= 0) {
   return;
  }
  colorBuffers = std::min(colorBuffers, colorCount);
  std::vector<int> drawBuffers;
  drawBuffers.reserve(static_cast<std::size_t>(colorBuffers));
  for(int i = 0; i < colorBuffers; ++i) {
   // ShadowRenderTargets.java:310).
   const unsigned int tex = shadowcolor[static_cast<std::size_t>(i)][flipped[static_cast<std::size_t>(i)] ? 0 : 1];
   fbo.addColorAttachment(i, tex);
   drawBuffers.push_back(i);
  }
  if(!fbo.drawBuffers(drawBuffers)) {
   return;
  }
}
void ShadowTargets::prepareForShadowRender() {
  if(!valid()) {
   return;
  }
  for(int i = 0; i < colorCount; ++i) {
   fbo.addColorAttachment(i, shadowcolor[static_cast<std::size_t>(i)][0]);
  }
}
void ShadowTargets::snapshotOpaqueDepth() {
  if(!valid()) {
   return;
  }
  if(shadowtex1 == 0) {
   shadowtex1 = static_cast<unsigned int>(core::genTexture());
   shadowtex1Resolution = 0;
  }
  core::bindTexture(static_cast<int>(shadowtex1));
  if(shadowtex1Resolution != resolution) {
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderTargets.java
   ::glTexImage2D(0x0DE1, 0, kDepthComponent32, resolution, resolution, 0, 0x1902, 0x1405, nullptr);
   ::glTexParameteri(0x0DE1, 0x2801, 0x2600);
   ::glTexParameteri(0x0DE1, 0x2800, 0x2600);
   ::glTexParameteri(0x0DE1, 0x2802, 0x812F);
   ::glTexParameteri(0x0DE1, 0x2803, 0x812F);
   applyDepthSwizzle(shadowtex1);
   shadowtex1Resolution = resolution;
  }
  ::glCopyTexSubImage2D(0x0DE1, 0, 0, 0, 0, 0, resolution, resolution);
}
void reset(ShadowMapState& state) {
  state.targets.destroy();
}
void snapshotOpaqueDepth(ShadowMapState& state) {
  state.targets.snapshotOpaqueDepth();
}
ShadowMapResult update(ShadowMapState& state,
                       GameRenderer& renderer,
                       float tickDelta,
                       const FrameRenderCamera& camera,
                       const PackDefinition& definition) {
  // Every value below comes straight from the definition. With no pack loaded that is
  // vanillaPackDefinition(), whose shadowMapResolution of 0 makes this function bail at
  // the guard below - so vanilla needs no fallback spelled out here. The previous
  // fallbacks had also drifted away from the struct's own defaults (near 0.05 vs
  // -100.05, far 256 vs 156, distance 0 vs 160), leaving two disagreeing definitions of
  // "vanilla".
  const int requestedResolution = definition.shadowMapResolution;
  const int colorBuffers = std::clamp(definition.shadowColorBuffers, 0, 8);
  const ShadowCullState cullState = definition.shadowCulling;
  const float voxelDistance = definition.voxelDistance;
  const float shadowDistance = definition.shadowDistance;
  const float shadowDistanceRenderMul = definition.shadowDistanceRenderMul;
  const float shadowMapFov = definition.shadowMapFov;
  const float shadowNearPlane = definition.shadowNearPlane;
  const float shadowFarPlane = definition.shadowFarPlane;
  const float shadowIntervalSize = definition.shadowIntervalSize;
  const float entityDistanceMultiplier = definition.entityShadowDistanceMul;
  const bool shadowEntities = definition.shadowEntities;
  const bool shadowPlayer = definition.shadowPlayer;
  const bool shadowTerrain = definition.shadowTerrain;
  const bool shadowTranslucent = definition.shadowTranslucent;
  const bool shadowBlockEntities = definition.shadowBlockEntities;
  const bool shadowLightBlockEntities = definition.shadowLightBlockEntities;
  if(requestedResolution <= 0 || renderer.client == nullptr || renderer.client->world == nullptr) {
   reset(state);
   return {};
  }
  // https://github.com/IrisShaders/Iris/issues/764
  if(shadowDistance <= 0.0f) {
   reset(state);
   return {};
  }
  const int resolution = std::clamp(requestedResolution, 256, 16384);
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  state.targets.depthCompare = definition.shadowHardwareFiltering[0] &&
                               !featureEnabled(definition, "SEPARATE_HARDWARE_SAMPLERS");
  if(!state.targets.ensure(resolution, colorBuffers)) {
   return {};
  }
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
  const float sunPathRotation = definition.sunPathRotation;
  const float celestialAngle = renderer.client->world->getTime(tickDelta);
  const float shadowAngle = shadowAngleFromCelestial(celestialAngle);
  const float renderDistanceBlocks = renderer.frameSettings().renderDistanceBlocks;
  const float effectiveShadowDistance = shadowDistanceRenderMul >= 0.0f
                                             ? shadowDistanceRenderMul * renderDistanceBlocks
                                             : shadowDistance;
  const float coverage = effectiveShadowDistance;
  const bool perspectiveShadow = shadowMapFov > 0.0f;
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
  const double centerX = camera.eyeX;
  const double centerY = camera.eyeY;
  const double centerZ = camera.eyeZ;
   const float planeScale = (shadowDistance > 0.0f && effectiveShadowDistance > 0.0f)
                                ? (effectiveShadowDistance / shadowDistance)
                                : 1.0f;
   const float nearZ = shadowNearPlane * planeScale;
   const float farZ = shadowFarPlane * planeScale;
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
  FrameRenderCamera shadowCam{};
  shadowCam.x = centerX;
  shadowCam.y = centerY;
  shadowCam.z = centerZ;
  shadowCam.eyeX = centerX;
  shadowCam.eyeY = centerY;
  shadowCam.eyeZ = centerZ;
  shadowCam.customView = true;
  shadowCam.hideFirstPersonHand = true;
  shadowCam.orthographic = !perspectiveShadow;
  shadowCam.orthoHalfWidth = coverage;
  shadowCam.orthoHalfHeight = coverage;
  shadowCam.orthoNear = nearZ;
  shadowCam.orthoFar = farZ;
  shadowCam.shadowPass = true;
  shadowCam.shadowEntities = shadowEntities;
  shadowCam.shadowPlayer = shadowPlayer;
  shadowCam.shadowTerrain = shadowTerrain;
  shadowCam.shadowTranslucent = shadowTranslucent;
  shadowCam.shadowBlockEntities = shadowBlockEntities;
  shadowCam.shadowLightBlockEntities = shadowLightBlockEntities;
  shadowCam.hasExplicitModelView = true;
  buildShadowCelestialModelView(shadowCam.explicitModelView, shadowAngle, sunPathRotation, shadowIntervalSize,
                                centerX, centerY, centerZ);
  float lightVector[3] = {shadowCam.explicitModelView[8], shadowCam.explicitModelView[9],
                          shadowCam.explicitModelView[10]};
  {
   const float length =
       std::sqrt(lightVector[0] * lightVector[0] + lightVector[1] * lightVector[1] + lightVector[2] * lightVector[2]);
   if(length > 1.0e-6f) {
    for(float& component : lightVector) component /= length;
   }
  }
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  float playerModelViewProjection[16]{};
  {
   const float playerFarPlane = renderer.frameSettings().renderDistanceBlocks;
   float playerProjection[16]{};
   float playerModelView[16]{};
   buildCameraProjection(playerProjection, camera, playerFarPlane);
   buildCameraModelView(playerModelView, camera);
   net::minecraft::util::math::Matrix4f composed;
   composed.set(playerProjection);
   net::minecraft::util::math::Matrix4f modelView;
   modelView.set(playerModelView);
   composed.multiply(modelView); // gbufferProjection * gbufferModelView
   std::memcpy(playerModelViewProjection, composed.data(), sizeof(float) * 16);
  }
  ShadowFrustumParams frustumParams;
  frustumParams.cullState = cullState;
  // Voxelization is assumed when the shadow program has a geometry stage; the vanilla
  // definition has no programs, so this is false without a pack-presence check.
  frustumParams.packHasVoxelization = [&] {
   const auto found = definition.programs.find("shadow");
   return found != definition.programs.end() && !found->second.geometry.empty();
  }();
  frustumParams.halfPlaneLength = effectiveShadowDistance;
  frustumParams.voxelDistance = voxelDistance;
  frustumParams.renderMultiplier = shadowDistanceRenderMul;
  frustumParams.renderDistanceBlocks = renderer.frameSettings().renderDistanceBlocks;
  state.terrainFrustum = createShadowFrustum(frustumParams, playerModelViewProjection, lightVector);
  state.terrainFrustum.prepare(centerX, centerY, centerZ);
  if(entityDistanceMultiplier == 1.0f || entityDistanceMultiplier < 0.0f) {
   state.entityFrustum = state.terrainFrustum;
  } else {
   ShadowFrustumParams entityParams = frustumParams;
   entityParams.renderMultiplier = shadowDistanceRenderMul * entityDistanceMultiplier;
   state.entityFrustum = createShadowFrustum(entityParams, playerModelViewProjection, lightVector);
  }
  state.entityFrustum.prepare(centerX, centerY, centerZ);
  shadowCam.shadowTerrainFrustum = &state.terrainFrustum;
  shadowCam.shadowEntityFrustum = &state.entityFrustum;
  if(perspectiveShadow) {
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
   shadowCam.perspectiveNear = std::max(0.05f, nearZ);
   shadowCam.perspectiveFar = 156.0f;
  }
  const float fov = perspectiveShadow ? shadowMapFov : 70.0f;
  state.targets.prepareForShadowRender();
  if(!renderer.renderWorldToFbo(state.targets.fbo.id(), resolution, resolution, tickDelta, shadowCam, fov,
                                &state.shadowCamera)) {
   return {};
  }
  ShadowMapResult result;
  result.depthTexture = static_cast<int>(state.targets.shadowtex0);
  result.opaqueDepthTexture =
      state.targets.shadowtex1 != 0 ? static_cast<int>(state.targets.shadowtex1) : result.depthTexture;
  result.resolution = resolution;
  result.colorCount = colorBuffers;
  for(int i = 0; i < colorBuffers; ++i) {
   result.colorTextures[static_cast<std::size_t>(i)] =
       static_cast<int>(state.targets.shadowcolor[static_cast<std::size_t>(i)][0]);
    result.colorAltTextures[static_cast<std::size_t>(i)] =
        static_cast<int>(state.targets.shadowcolor[static_cast<std::size_t>(i)][1]);
   }
  return result;
 }
} // namespace net::minecraft::client::render::shadowmap
