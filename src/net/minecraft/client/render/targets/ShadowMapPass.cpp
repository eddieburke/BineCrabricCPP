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
constexpr unsigned kTextureDepthSize = 0x884A;
constexpr int kSwizzleR[] = {0x1903, 0x1903, 0x1903, 0x1404};
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
  fbo.noDrawBuffers();
  ::glReadBuffer(0);
 }
 shadowtex0 = core::genTexture();
 core::bindTexture(gl::cap::Texture2D, static_cast<int>(shadowtex0));
 ::glTexImage2D(gl::cap::Texture2D, 0, kDepthComponent32, resolution, resolution, 0, gl::pixel::DepthComponent,
                gl::pixel::UnsignedInt, nullptr);
 {
  int depthBits = 0;
  ::glGetTexLevelParameteriv(gl::cap::Texture2D, 0, static_cast<unsigned>(kTextureDepthSize), &depthBits);
  shadowDepthBits = depthBits > 0 ? depthBits : 24;
 }
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
// The shadow camera is the pack's shadowDistance/planes turned into the one camera
// shape the renderer knows: an orthographic box centred on the player's eye whose
// half-extent is shadowDistance, clipped by shadowNearPlane/shadowFarPlane. A pack
// with shadowMapFov > 0 gets a perspective shadow camera instead, with the pack's
// fov as its focal terms. This is the only place the pack's shadow tuning becomes a
// camera; nothing downstream re-derives it.
FrameRenderCamera makeShadowCamera(const PackDefinition& definition,
                                   const FrameRenderCamera& camera,
                                   const CelestialState& celestial) {
 const bool perspectiveShadow = definition.shadowMapFov > 0.0f;
 FrameRenderCamera shadowCam{};
 shadowCam.x = camera.eyeX;
 shadowCam.y = camera.eyeY;
 shadowCam.z = camera.eyeZ;
 shadowCam.eyeX = camera.eyeX;
 shadowCam.eyeY = camera.eyeY;
 shadowCam.eyeZ = camera.eyeZ;
 shadowCam.customView = true;
 shadowCam.hideFirstPersonHand = true;
 shadowCam.orthographic = !perspectiveShadow;
 shadowCam.orthoHalfWidth = definition.shadowDistance;
 shadowCam.orthoHalfHeight = definition.shadowDistance;
 shadowCam.nearPlane = definition.shadowNearPlane;
 shadowCam.farPlane = definition.shadowFarPlane;
 shadowCam.shadowPass = true;
 shadowCam.shadowEntities = definition.shadowEntities;
 shadowCam.shadowPlayer = definition.shadowPlayer;
 shadowCam.shadowTerrain = definition.shadowTerrain;
 shadowCam.shadowTranslucent = definition.shadowTranslucent;
 shadowCam.shadowBlockEntities = definition.shadowBlockEntities;
 shadowCam.shadowLightBlockEntities = definition.shadowLightBlockEntities;
 shadowCam.hasExplicitModelView = true;
 buildShadowCelestialModelView(shadowCam.explicitModelView, celestial.shadowAngle,
                               celestial.sunPathRotation, definition.shadowIntervalSize, camera.eyeX,
                               camera.eyeY, camera.eyeZ);
 if(perspectiveShadow) {
  shadowCam.nearPlane = std::max(0.05f, definition.shadowNearPlane);
  shadowCam.farPlane = 156.0f;
  const float yScale = 1.0f / std::tan(definition.shadowMapFov * 3.14159265f / 360.0f);
  shadowCam.projectionX = yScale;
  shadowCam.projectionY = yScale;
 }
 return shadowCam;
}
ShadowMapResult update(ShadowMapState& state,
                       GameRenderer& renderer,
                       float tickDelta,
                       const FrameRenderCamera& camera,
                       const PackDefinition& definition) {
 const int requestedResolution = definition.shadowMapResolution;
 const int colorBuffers = std::clamp(definition.shadowColorBuffers, 0, 8);
 const ShadowCullState cullState = definition.shadowCulling;
 const float voxelDistance = definition.voxelDistance;
 const float shadowDistance = definition.shadowDistance;
 const float shadowDistanceRenderMul = definition.shadowDistanceRenderMul;
 const float entityDistanceMultiplier = definition.entityShadowDistanceMul;
 if(requestedResolution <= 0 || renderer.client == nullptr || renderer.client->world == nullptr) {
  state.targets.destroy();
  return {};
 }
 if(shadowDistance <= 0.0f) {
  state.targets.destroy();
  return {};
 }
 const int resolution = std::clamp(requestedResolution, 256, 16384);
 state.targets.depthCompare = definition.shadowHardwareFiltering[0] &&
                              !featureEnabled(definition, "SEPARATE_HARDWARE_SAMPLERS");
 if(!state.targets.ensure(resolution, colorBuffers)) {
  return {};
 }
 const CelestialState& celestial = core::celestialState();
 const double centerX = camera.eyeX;
 const double centerY = camera.eyeY;
 const double centerZ = camera.eyeZ;
 FrameRenderCamera shadowCam = makeShadowCamera(definition, camera, celestial);
 shadowCam.shadowDepthBits = state.targets.shadowDepthBits;
 float lightVector[3] = {celestial.shadowLightDirectionWorld[0], celestial.shadowLightDirectionWorld[1],
                         celestial.shadowLightDirectionWorld[2]};
 float playerModelViewProjection[16]{};
 {
  float playerProjection[16]{};
  float playerModelView[16]{};
  buildCameraProjection(playerProjection, camera);
  buildCameraModelView(playerModelView, camera);
  net::minecraft::util::math::Matrix4f composed;
  composed.set(playerProjection);
  net::minecraft::util::math::Matrix4f modelView;
  modelView.set(playerModelView);
  composed.multiply(modelView);
  std::memcpy(playerModelViewProjection, composed.data(), sizeof(float) * 16);
 }
 ShadowFrustumParams frustumParams;
 frustumParams.cullState = cullState;
 frustumParams.packHasVoxelization = [&] {
  const auto found = definition.programs.find("shadow");
  return found != definition.programs.end() && !found->second.geometry.empty();
 }();
 frustumParams.halfPlaneLength = shadowDistance;
 frustumParams.voxelDistance = voxelDistance;
 frustumParams.renderMultiplier = shadowDistanceRenderMul;
 frustumParams.renderDistanceBlocks = renderer.frameSettings().renderDistance.ringBlocks();
 // shadowDistanceRenderMul < 0 means "cull to the user's render distance" (Iris
 // ShadowRenderer.createShadowFrustum); createShadowFrustum reads it back off
 // renderDistanceBlocks, so the ring and the cull distance share one source.
 frustumParams.forceBoxCull = renderer.client != nullptr && renderer.client->options.shadowForceBoxCull;
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
 const float fov = shadowCam.orthographic ? 70.0f : definition.shadowMapFov;
 state.targets.prepareForShadowRender();
 if(!renderer.renderWorldToFbo(state.targets.fbo.id(), resolution, resolution, tickDelta, shadowCam, fov)) {
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
