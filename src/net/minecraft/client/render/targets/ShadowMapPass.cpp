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
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
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
 compositeAttach = {};
 compositeDrawBufferCount = -1;
 colorFormats = {};
 fullClearPending = false;
}
bool ShadowTargets::ensure(int resolutionIn, int colorBuffers, const PackDefinition& definition) {
 colorBuffers = std::clamp(colorBuffers, 0, 8);
 std::array<ColorFormat, 8> wantedFormats{};
 for(int i = 0; i < colorBuffers; ++i) {
  const auto target = definition.targets.find("shadowcolor" + std::to_string(i));
  wantedFormats[static_cast<std::size_t>(i)] =
      target == definition.targets.end() ? ColorFormat::Rgba8 : parseFormat(target->second.format);
 }
 if(valid() && resolution == resolutionIn && colorCount == colorBuffers && colorFormats == wantedFormats) {
  return true;
 }
 destroy();
 gl::GLCore::ensureLoaded();
 if(!gl::GLCore::framebufferSupported || resolutionIn <= 0) {
  return false;
 }
 resolution = resolutionIn;
 colorCount = colorBuffers;
 colorFormats = wantedFormats;
 fullClearPending = true;
 const core::TextureBindScope textureScope;
 std::vector<int> drawBuffers;
 drawBuffers.reserve(static_cast<std::size_t>(colorBuffers));
 for(int i = 0; i < colorBuffers; ++i) {
  const GlFormat format = glFormat(colorFormats[static_cast<std::size_t>(i)]);
  for(unsigned int& tex : shadowcolor[static_cast<std::size_t>(i)]) {
   tex = core::genTexture();
   core::bindTexture(gl::cap::Texture2D, static_cast<int>(tex));
   ::glTexImage2D(gl::cap::Texture2D, 0, format.internal, resolution, resolution, 0, format.format,
                  format.type, nullptr);
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
 std::array<unsigned int, 8> want{};
 bool changed = compositeDrawBufferCount != colorBuffers;
 for(int i = 0; i < colorBuffers; ++i) {
  want[static_cast<std::size_t>(i)] =
      shadowcolor[static_cast<std::size_t>(i)][flipped[static_cast<std::size_t>(i)] ? 0 : 1];
  if(want[static_cast<std::size_t>(i)] != compositeAttach[static_cast<std::size_t>(i)]) {
   changed = true;
  }
 }
 if(!changed) {
  return;
 }
 std::vector<int> drawBuffers;
 drawBuffers.reserve(static_cast<std::size_t>(colorBuffers));
 for(int i = 0; i < colorBuffers; ++i) {
  fbo.addColorAttachment(i, want[static_cast<std::size_t>(i)]);
  drawBuffers.push_back(i);
 }
 if(!fbo.drawBuffers(drawBuffers)) {
  return;
 }
 compositeAttach = want;
 compositeDrawBufferCount = colorBuffers;
}
void ShadowTargets::prepareForShadowRender(const PackDefinition& definition) {
 if(!valid()) {
  return;
 }
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, fbo.id());
 std::vector<int> drawBuffers;
 drawBuffers.reserve(static_cast<std::size_t>(colorCount));
 for(int i = 0; i < colorCount; ++i) {
  drawBuffers.push_back(i);
 }
 for(int side = 0; side < 2; ++side) {
  for(int i = 0; i < colorCount; ++i) {
   fbo.addColorAttachment(i, shadowcolor[static_cast<std::size_t>(i)][static_cast<std::size_t>(side)]);
  }
  if(colorCount > 0) {
   fbo.drawBuffers(drawBuffers);
  }
  for(int i = 0; i < colorCount; ++i) {
   const auto target = definition.targets.find("shadowcolor" + std::to_string(i));
   if(!fullClearPending && target != definition.targets.end() && !target->second.clear) {
    continue;
   }
   std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
   if(target != definition.targets.end() && target->second.customClearColor) {
    std::copy(std::begin(target->second.clearColor), std::end(target->second.clearColor), color.begin());
   }
   const ColorFormat format = colorFormats[static_cast<std::size_t>(i)];
   if(isIntegerColorFormat(format)) {
    if(isSignedIntegerColorFormat(format) && gl::GLCore::clearBufferiv != nullptr) {
     const int value[4] = {static_cast<int>(color[0]), static_cast<int>(color[1]),
                           static_cast<int>(color[2]), static_cast<int>(color[3])};
     gl::GLCore::clearBufferiv(gl::framebuffer::Color, i, value);
    } else if(gl::GLCore::clearBufferuiv != nullptr) {
     const unsigned value[4] = {static_cast<unsigned>(color[0]), static_cast<unsigned>(color[1]),
                                static_cast<unsigned>(color[2]), static_cast<unsigned>(color[3])};
     gl::GLCore::clearBufferuiv(gl::framebuffer::Color, i, value);
    }
   } else if(gl::GLCore::clearBufferfv != nullptr) {
    gl::GLCore::clearBufferfv(gl::framebuffer::Color, i, color.data());
   }
  }
 }
 fullClearPending = false;
 for(int i = 0; i < colorCount; ++i) {
  fbo.addColorAttachment(i, shadowcolor[static_cast<std::size_t>(i)][0]);
 }
 if(colorCount > 0) {
  fbo.drawBuffers(drawBuffers);
 }
 compositeDrawBufferCount = -1;
}
// see third_party/mcp/iris/gl/texture/DepthCopyStrategy.java
// Both textures are DEPTH_COMPONENT32 at `resolution`, so the GL43 image copy is
// legal between them: texture to texture, no framebuffer read and no binding.
// glCopyTexSubImage2D is the Gl20CopyTexture fallback, and like Java's it has to
// put the previous binding back - the copy is not allowed to disturb texture state.
void ShadowTargets::snapshotOpaqueDepth() {
 if(!valid()) {
  return;
 }
 const int previousTexture = core::boundTexture();
 if(shadowtex1 == 0) {
  shadowtex1 = static_cast<unsigned int>(core::genTexture());
  shadowtex1Resolution = 0;
 }
 if(shadowtex1Resolution != resolution) {
  core::bindTexture(static_cast<int>(shadowtex1));
  ::glTexImage2D(gl::cap::Texture2D, 0, kDepthComponent32, resolution, resolution, 0,
                 gl::pixel::DepthComponent, gl::pixel::UnsignedInt, nullptr);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Nearest);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Nearest);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
  applyDepthSwizzle(shadowtex1);
  shadowtex1Resolution = resolution;
 }
 if(gl::GLCore::copyImageSubData != nullptr) {
  gl::GLCore::copyImageSubData(shadowtex0, gl::cap::Texture2D, 0, 0, 0, 0,
                               shadowtex1, gl::cap::Texture2D, 0, 0, 0, 0,
                               resolution, resolution, 1);
 } else {
  core::bindTexture(static_cast<int>(shadowtex1));
  ::glCopyTexSubImage2D(gl::cap::Texture2D, 0, 0, 0, 0, 0, resolution, resolution);
 }
 core::bindTexture(previousTexture);
}
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
 if(celestial.directionOverride) {
  buildShadowDirectionalModelView(shadowCam.explicitModelView,
                                  celestial.shadowLightDirectionWorld,
                                  definition.shadowIntervalSize,
                                  camera.eyeX,
                                  camera.eyeY,
                                  camera.eyeZ);
 } else {
  buildShadowCelestialModelView(shadowCam.explicitModelView, celestial.shadowAngle,
                                celestial.sunPathRotation, definition.shadowIntervalSize, camera.eyeX,
                                camera.eyeY, camera.eyeZ);
 }
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
 const float voxelDistance = definition.effectiveVoxelDistance();
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
 if(!state.targets.ensure(resolution, colorBuffers, definition)) {
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
 frustumParams.renderDistanceBlocks = renderer.frameSettings().renderDistance.sectionCoverageBlocks();
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
 state.targets.prepareForShadowRender(definition);
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
