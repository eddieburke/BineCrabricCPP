#include "net/minecraft/client/render/targets/ShadowMapPass.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
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
// Iris' depth texture swizzle: old packs expect the depth in .z, but the driver
// only guarantees it in .r, so replicate the red channel into rgb and set a=1.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
// (configureDepthSampler, ARBTextureSwizzle.GL_TEXTURE_SWIZZLE_RGBA)
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
   // Iris creates shadowcolor targets with main + alt textures; the shadow map
   // terrain render always writes the main texture (createShadowFramebuffer with an
   // empty flip set), while shadow composite passes follow the per-pass flip state.
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderTargets.java
   for(unsigned int& tex : shadowcolor[static_cast<std::size_t>(i)]) {
    tex = core::genTexture();
    core::bindTexture(gl::cap::Texture2D, static_cast<int>(tex));
    // Sized GL_RGBA8 — unsized GL_RGBA internal → GL_INVALID_ENUM on forward-compat core.
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
   // Java createShadowFramebuffer with zero color buffers: noDrawBuffers + no read
   // buffer (GlFramebuffer.java:57-59).
   fbo.noDrawBuffers();
   ::glReadBuffer(0);
  }
  shadowtex0 = core::genTexture();
  core::bindTexture(gl::cap::Texture2D, static_cast<int>(shadowtex0));
  // Iris allocates the shadow depth as DEPTH32 (TextureFormat.DEPTH32).
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
   // Shadow composite passes write the OPPOSITE side from the one they sample: Iris
   // passes the flipped snapshot as stageWritesToMain, so a flipped buffer attaches its
   // main texture and an unflipped one its alt texture (createColorFramebuffer,
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
  // Re-attach the main textures for buffers whose attachment may have been switched
  // to the alt side by a shadow composite pass (attachCompositeColors). The shadow
  // map terrain render always writes the main textures, like Iris' shadow
  // framebuffers created with an empty flip set.
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
   // DEPTH32 like Iris' "Shadow Map / Opaque" texture.
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
                       const PackDefinition* definition) {
  const int requestedResolution = definition != nullptr ? definition->shadowMapResolution : 0;
  const int colorBuffers = std::clamp(definition != nullptr ? definition->shadowColorBuffers : 0, 0, 8);
  const bool shadowCulling = definition == nullptr || definition->shadowCulling;
  const bool reversedCulling = definition != nullptr && definition->reversedShadowCulling;
  const float voxelDistance = definition != nullptr ? definition->voxelDistance : 0.0f;
  const float shadowDistance = definition != nullptr ? definition->shadowDistance : 0.0f;
  const float shadowDistanceRenderMul = definition != nullptr ? definition->shadowDistanceRenderMul : -1.0f;
  const float shadowMapFov = definition != nullptr ? definition->shadowMapFov : 0.0f;
  const float shadowNearPlane = definition != nullptr ? definition->shadowNearPlane : 0.05f;
  const float shadowFarPlane = definition != nullptr ? definition->shadowFarPlane : 256.0f;
  const float shadowIntervalSize = definition != nullptr ? definition->shadowIntervalSize : 2.0f;
  const float entityDistanceMultiplier = definition != nullptr ? definition->entityShadowDistanceMul : 1.0f;
  const bool shadowEntities = definition == nullptr || definition->shadowEntities;
  // Java: shadow.player is independent of shadow.entities and defaults to false.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/PackShadowDirectives.java
  const bool shadowPlayer = definition != nullptr && definition->shadowPlayer;
  const bool shadowTerrain = definition == nullptr || definition->shadowTerrain;
  const bool shadowTranslucent = definition == nullptr || definition->shadowTranslucent;
  const bool shadowBlockEntities = definition == nullptr || definition->shadowBlockEntities;
  const bool shadowLightBlockEntities = definition == nullptr || definition->shadowLightBlockEntities;
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
  // Java only enables texture-level compare mode when hardware filtering is requested
  // without separate hardware samplers (configureDepthSampler); the same flag feeds
  // the sampler objects used elsewhere in the C++ path. Set before ensure() so freshly
  // created depth textures get the right compare state.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  state.targets.depthCompare = definition != nullptr && definition->shadowHardwareFiltering[0] &&
                               !featureEnabled(*definition, "SEPARATE_HARDWARE_SAMPLERS");
  if(!state.targets.ensure(resolution, colorBuffers)) {
   return {};
  }
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
  const float sunPathRotation = definition != nullptr ? definition->sunPathRotation : 0.0f;
  const float celestialAngle = renderer.client->world->getTime(tickDelta);
  // Java: shadowAngle = getSunAngle(isDay())/360 (CelestialUniforms), the same input
  // the shadowAngle uniform receives; the model view is built from it.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
  const float shadowAngle = shadowAngleFromCelestial(celestialAngle);
  // Java: the shadow map half plane is shadowDistance alone; voxelDistance only affects
  // culling. https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/PackShadowDirectives.java
  const float coverage = shadowDistance;
  const float entityDistance = entityDistanceMultiplier > 0.0f ? coverage * entityDistanceMultiplier : 0.0f;
  float frustumBypass = 0.0f;
  if(!shadowCulling) {
   frustumBypass = coverage;
  } else if(reversedCulling) {
   frustumBypass = std::max(0.0f, voxelDistance);
  }
  float renderDistance = 0.0f;
  if(shadowDistanceRenderMul > 0.0f && shadowDistance > 0.0f) {
   renderDistance = shadowDistance * shadowDistanceRenderMul;
  }
  const bool perspectiveShadow = shadowMapFov > 0.0f;
  // Entity/terrain offsets use the player camera, not a light-eye translation.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  const double centerX = camera.x;
  const double centerY = camera.y;
  const double centerZ = camera.z;
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
  // Ortho planes come straight from the pack directives; the defaults ARE the
  // Java constants (NEAR = -100.05f; FAR = 156.0f), so no engine-side mapping
  // is needed here. (Perspective shadows always use the fixed FAR below.)
  const float nearZ = shadowNearPlane;
  const float farZ = shadowFarPlane;
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
  shadowCam.shadowEntityDistance = std::max(0.0f, entityDistance);
  shadowCam.frustumBypassDistance = std::max(0.0f, frustumBypass);
  shadowCam.shadowRenderDistance = std::max(0.0f, renderDistance);
  shadowCam.hasExplicitModelView = true;
  buildShadowCelestialModelView(shadowCam.explicitModelView, shadowAngle, sunPathRotation, shadowIntervalSize,
                                centerX, centerY, centerZ);
  if(perspectiveShadow) {
   // Iris' perspective shadow projection always uses the fixed NEAR/FAR constants.
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
