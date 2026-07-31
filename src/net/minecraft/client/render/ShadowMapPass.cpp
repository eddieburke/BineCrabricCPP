#include "net/minecraft/client/render/ShadowMapPass.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPack.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::client::render::shadowmap {
namespace {
constexpr unsigned kTextureCompareMode = 0x884C;
constexpr unsigned kTextureCompareFunc = 0x884D;
constexpr int kCompareRefToTexture = 0x884E;
constexpr int kLessEqual = 0x0203;
constexpr int kNone = 0;
} // namespace
void ShadowTargets::destroy() {
 if(shadowtex1 != 0) {
  core::deleteTexture(shadowtex1);
  shadowtex1 = 0;
 }
 shadowtex1Resolution = 0;
 for(unsigned int& tex : shadowcolor) {
  if(tex != 0) {
   core::deleteTexture(tex);
   tex = 0;
  }
 }
 if(shadowtex0 != 0) {
  core::deleteTexture(shadowtex0);
  shadowtex0 = 0;
 }
 if(fbo != 0) {
  gl::GLCore::deleteFramebuffers(1, &fbo);
  fbo = 0;
 }
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
 gl::GLCore::genFramebuffers(1, &fbo);
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
 const core::TextureBindScope textureScope;
 std::vector<unsigned int> drawBuffers;
 drawBuffers.reserve(static_cast<std::size_t>(colorBuffers));
 for(int i = 0; i < colorBuffers; ++i) {
  shadowcolor[static_cast<std::size_t>(i)] = core::genTexture();
  core::bindTexture(gl::cap::Texture2D, static_cast<int>(shadowcolor[static_cast<std::size_t>(i)]));
  // Sized GL_RGBA8 — unsized GL_RGBA internal → GL_INVALID_ENUM on forward-compat core.
  ::glTexImage2D(gl::cap::Texture2D, 0, gl::pixel::Rgba8, resolution, resolution, 0, gl::pixel::Rgba,
                 gl::pixel::UnsignedByte, nullptr);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Linear);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Linear);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
  gl::GLCore::framebufferTexture2D(gl::framebuffer::Framebuffer,
                                   static_cast<unsigned>(gl::framebuffer::ColorAttachment0 + i),
                                   gl::cap::Texture2D, shadowcolor[static_cast<std::size_t>(i)], 0);
  drawBuffers.push_back(static_cast<unsigned>(gl::framebuffer::ColorAttachment0 + i));
 }
 if(colorBuffers > 0) {
  if(gl::GLCore::drawBuffers != nullptr) {
   gl::GLCore::drawBuffers(colorBuffers, drawBuffers.data());
  }
 } else {
  ::glDrawBuffer(0);
  ::glReadBuffer(0);
 }
 shadowtex0 = core::genTexture();
 core::bindTexture(gl::cap::Texture2D, static_cast<int>(shadowtex0));
 ::glTexImage2D(gl::cap::Texture2D, 0, gl::pixel::DepthComponent24, resolution, resolution, 0,
                gl::pixel::DepthComponent, gl::pixel::UnsignedInt, nullptr);
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
 gl::GLCore::framebufferTexture2D(gl::framebuffer::Framebuffer, gl::framebuffer::DepthAttachment,
                                  gl::cap::Texture2D, shadowtex0, 0);
 const bool ok = gl::GLCore::checkFramebufferStatus(gl::framebuffer::Framebuffer) ==
                 static_cast<unsigned>(gl::framebuffer::Complete);
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, 0);
 if(!ok) {
  destroy();
  return false;
 }
 return true;
}
void ShadowTargets::bind() const {
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
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
  ::glTexImage2D(0x0DE1, 0, 0x81A6, resolution, resolution, 0, 0x1902, 0x1405, nullptr);
  ::glTexParameteri(0x0DE1, 0x2801, 0x2600);
  ::glTexParameteri(0x0DE1, 0x2800, 0x2600);
  ::glTexParameteri(0x0DE1, 0x2802, 0x812F);
  ::glTexParameteri(0x0DE1, 0x2803, 0x812F);
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
                       float farPlane,
                       const shaderpack::ShaderPackDefinition* definition) {
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
 const bool shadowPlayer = definition == nullptr || definition->shadowEntities || definition->shadowPlayer;
 const bool shadowTerrain = definition == nullptr || definition->shadowTerrain;
 const bool shadowTranslucent = definition == nullptr || definition->shadowTranslucent;
 const bool shadowBlockEntities = definition == nullptr || definition->shadowBlockEntities;
 const bool shadowLightBlockEntities = definition == nullptr || definition->shadowLightBlockEntities;
 if(requestedResolution <= 0 || renderer.client == nullptr || renderer.client->world == nullptr) {
  reset(state);
  return {};
 }
 const int resolution = std::clamp(requestedResolution, 256, 16384);
 if(!state.targets.ensure(resolution, colorBuffers)) {
  return {};
 }
 const auto& sun = renderer.client->world->lightRegistry().sun();
 const float length =
     std::sqrt(sun.directionX * sun.directionX + sun.directionY * sun.directionY + sun.directionZ * sun.directionZ);
 if(length <= 0.0001f || sun.intensity <= 0.0f) {
  return {};
 }
 float sx = sun.directionX / length, sy = sun.directionY / length, sz = sun.directionZ / length;
 if(definition != nullptr && definition->sunPathRotation != 0.0f) {
  const float rad = definition->sunPathRotation * 0.017453292519943295f;
  const float c = std::cos(rad), s = std::sin(rad);
  const float ny = sy * c - sz * s, nz = sy * s + sz * c;
  sy = ny;
  sz = nz;
 }
 const float coverage = shadowDistance > 0.0f ? std::max(shadowDistance, voxelDistance)
                                              : std::max(48.0f, std::min(farPlane, 192.0f) * 0.72f);
 const float distance = coverage * 1.45f;
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
 double centerX = camera.x, centerY = camera.y, centerZ = camera.z;
 if(shadowIntervalSize > 0.0f) {
  const double interval = static_cast<double>(shadowIntervalSize);
  centerX = std::floor(centerX / interval) * interval;
  centerY = std::floor(centerY / interval) * interval;
  centerZ = std::floor(centerZ / interval) * interval;
 }
 const float nearZ = std::max(0.0f, shadowNearPlane);
 const float farZ = shadowFarPlane > nearZ ? shadowFarPlane : nearZ + 1.0f;
 FrameRenderCamera shadowCam{};
 shadowCam.x = centerX + sx * distance;
 shadowCam.y = centerY + sy * distance;
 shadowCam.z = centerZ + sz * distance;
 shadowCam.yaw = std::atan2(sx, -sz) * 57.29577951308232f;
 shadowCam.pitch = std::asin(std::clamp(sy, -1.0f, 1.0f)) * 57.29577951308232f;
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
 if(perspectiveShadow) {
  shadowCam.perspectiveNear = nearZ;
  shadowCam.perspectiveFar = farZ;
 }
 const float fov = perspectiveShadow ? shadowMapFov : 70.0f;
 if(!renderer.renderWorldToFbo(state.targets.fbo, resolution, resolution, tickDelta, shadowCam, fov,
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
      static_cast<int>(state.targets.shadowcolor[static_cast<std::size_t>(i)]);
 }
 return result;
}
} // namespace net::minecraft::client::render::shadowmap
