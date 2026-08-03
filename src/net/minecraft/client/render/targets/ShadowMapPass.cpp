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
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
  const float shadowAngle = shadowAngleFromCelestial(celestialAngle);
  // culling. https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/PackShadowDirectives.java
  const float coverage = shadowDistance;
  const bool perspectiveShadow = shadowMapFov > 0.0f;
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
  const double centerX = camera.eyeX;
  const double centerY = camera.eyeY;
  const double centerZ = camera.eyeZ;
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
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
  shadowCam.hasExplicitModelView = true;
  buildShadowCelestialModelView(shadowCam.explicitModelView, shadowAngle, sunPathRotation, shadowIntervalSize,
                                centerX, centerY, centerZ);
  float lightVector[3] = {shadowCam.explicitModelView[2], shadowCam.explicitModelView[6],
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
  frustumParams.halfPlaneLength = shadowDistance;
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
  // TEMP DIAGNOSTIC — [shadow-probe]. Answers, in order: did anything land in the shadow
  // map, do the directives the engine resolved match what the GLSL was compiled with,
  // and is the map oriented the way the pack assumes. DELETE after investigation.
  {
   static int shadowProbeFrame = 0;
   if((shadowProbeFrame++ % 120) == 0) {
    // 1. Depth readback. The pack writes gl_Position.z itself, so a map that is
    // entirely 1.0 means nothing rasterised (culled away, or the program failed) —
    // which looks identical on screen to a badly oriented map.
    constexpr int kPatch = 32;
    std::vector<float> depth(static_cast<std::size_t>(kPatch * kPatch), 1.0f);
    const int origin = std::max(0, resolution / 2 - kPatch / 2);
    gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, state.targets.fbo.id());
    ::glReadPixels(origin, origin, kPatch, kPatch, gl::pixel::DepthComponent, 0x1406, depth.data());
    gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, 0);
    float minDepth = 1.0f;
    float maxDepth = 0.0f;
    double sum = 0.0;
    int written = 0;
    for(const float d : depth) {
     minDepth = std::min(minDepth, d);
     maxDepth = std::max(maxDepth, d);
     sum += static_cast<double>(d);
     if(d < 0.999f) ++written;
    }
    const int total = kPatch * kPatch;
    const int drawnRegions =
        renderer.client != nullptr && renderer.client->worldRenderer != nullptr
            ? renderer.client->worldRenderer->lastDrawnRegions()
            : -1;
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[shadow-probe] map res=") + std::to_string(resolution) + " centre " +
            std::to_string(kPatch) + "x" + std::to_string(kPatch) + " depth min=" +
            std::to_string(minDepth) + " max=" + std::to_string(maxDepth) + " mean=" +
            std::to_string(sum / total) + " written=" + std::to_string(written) + "/" +
            std::to_string(total) + (written == 0 ? "  <-- MAP IS EMPTY" : "") +
            " shadowRegionsDrawn=" + std::to_string(drawnRegions));
    // 2. Directives. shadowDistance/near/far must equal the pack's own consts, or the
    // pack's shadow_proj_scale and the engine disagree about the volume.
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[shadow-probe] shadowDistance=") + std::to_string(shadowDistance) +
            " renderMul=" + std::to_string(shadowDistanceRenderMul) + " near=" +
            std::to_string(shadowNearPlane) + " far=" + std::to_string(shadowFarPlane) +
            " intervalSize=" + std::to_string(shadowIntervalSize) + " sunPathRotation=" +
            std::to_string(sunPathRotation) + " fov=" + std::to_string(shadowMapFov) +
            " colorBuffers=" + std::to_string(colorBuffers) + " hwFilter=" +
            std::to_string(static_cast<int>(definition.shadowHardwareFiltering[0])) +
            " depthCompare=" + std::to_string(static_cast<int>(state.targets.depthCompare)));
    // 3. The matrix itself, column-major, as the four columns the shader receives.
    const float* m = shadowCam.explicitModelView;
    const auto col = [&](int c) {
     return std::string("(") + std::to_string(m[c * 4 + 0]) + "," + std::to_string(m[c * 4 + 1]) +
            "," + std::to_string(m[c * 4 + 2]) + "," + std::to_string(m[c * 4 + 3]) + ")";
    };
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[shadow-probe] shadowModelView c0=") + col(0) + " c1=" + col(1) +
            " c2=" + col(2) + " c3=" + col(3));
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[shadow-probe] lightVec=(") + std::to_string(lightVector[0]) + "," +
            std::to_string(lightVector[1]) + "," + std::to_string(lightVector[2]) +
            ") translation=(" + std::to_string(m[12]) + "," + std::to_string(m[13]) + "," +
            std::to_string(m[14]) + ")" +
            ((m[12] != 0.0f || m[13] != 0.0f || m[14] != 0.0f)
                 ? "  <-- NONZERO, pack's shadow.vsh drops it via mat3()"
                 : "") +
            " cull=" + std::to_string(static_cast<int>(state.terrainFrustum.mode())) + " planes=" +
            std::to_string(state.terrainFrustum.planeCount()));
    // 4. Resolution agreement. The engine allocates the texture, but the pack's PCF uses
    // its own compiled `shadowMapResolution` for tap offsets and the 64/res slope bias.
    // scanOptions skips shadowMapResolution, so a profile/user override reaches the GLSL
    // but never the engine — every PCF tap then lands at the wrong texel pitch.
    std::string optionRes = "<none>";
    std::string optionSmDist = "<none>";
    if(renderer.shaderPacks() != nullptr) {
     const std::string r = renderer.shaderPacks()->settingValue("shadowMapResolution");
     const std::string d = renderer.shaderPacks()->settingValue("SM_DIST");
     if(!r.empty()) optionRes = r;
     if(!d.empty()) optionSmDist = d;
    }
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[shadow-probe] resolution engine=") + std::to_string(resolution) +
            " allocated=" + std::to_string(state.targets.resolution) + " optionValue=" + optionRes +
            " SM_DIST=" + optionSmDist +
            ((optionRes != "<none>" && optionRes != std::to_string(resolution))
                 ? "  <-- MISMATCH, shader PCF pitch != texture size"
                 : ""));
    // 5. Round-trip. Push camera-relative points through exactly what the pack does:
    //   s_view = rot_trans_mmul(shadowModelView, pe)
    //   s_ndc  = vec3(1/shadowDistance, 1/shadowDistance, -2/(far-near)) * s_view
    //   s_ndc.xy *= distortion(s_ndc.xy);  uv = s_ndc*0.5 + 0.5
    // Anything landing outside [0,1] in uv, or outside [-1,1] in z, is sampling off-map.
    {
     const float sxy = shadowDistance != 0.0f ? 1.0f / shadowDistance : 0.0f;
     const float sz = (shadowFarPlane - shadowNearPlane) != 0.0f
                          ? -2.0f / (shadowFarPlane - shadowNearPlane)
                          : 0.0f;
     float distortStrength = 0.9f; // SM_DISTORTION 90 -> *0.01
     if(renderer.shaderPacks() != nullptr) {
      const std::string sd = renderer.shaderPacks()->settingValue("SM_DISTORTION");
      if(!sd.empty()) distortStrength = std::strtof(sd.c_str(), nullptr) * 0.01f;
     }
     const float squareness = shadowDistance != 0.0f ? 1.0f - 2.0f / shadowDistance : 0.0f;
     const auto distortion = [&](float x, float y) {
      const float a = x * x + y * y;
      const float inner = a * a - 4.0f * squareness * squareness * x * x * y * y;
      const float fg = 0.70710678f * std::sqrt(a + std::sqrt(std::max(0.0f, inner)));
      const float denom = fg * distortStrength + (1.0f - distortStrength);
      return denom != 0.0f ? 1.0f / denom : 0.0f;
     };
     const float points[][3] = {
         {0.0f, -2.0f, 0.0f}, {0.0f, -2.0f, -16.0f}, {0.0f, -2.0f, -64.0f},
         {0.0f, -2.0f, -140.0f}, {64.0f, -2.0f, 64.0f}, {0.0f, -2.0f, -300.0f}};
     for(const auto& pe : points) {
      const float vx = m[0] * pe[0] + m[4] * pe[1] + m[8] * pe[2] + m[12];
      const float vy = m[1] * pe[0] + m[5] * pe[1] + m[9] * pe[2] + m[13];
      const float vz = m[2] * pe[0] + m[6] * pe[1] + m[10] * pe[2] + m[14];
      float nx = vx * sxy;
      float ny = vy * sxy;
      const float nz = vz * sz;
      const float d = distortion(nx, ny);
      nx *= d;
      ny *= d;
      const float u = nx * 0.5f + 0.5f;
      const float v = ny * 0.5f + 0.5f;
      const bool off = u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f || nz < -1.0f || nz > 1.0f;
      ::net::minecraft::client::ClientLog::LOGGER.log(
          ::net::minecraft::util::logging::LogLevel::Info,
          std::string("[shadow-probe] pe=(") + std::to_string(pe[0]) + "," + std::to_string(pe[1]) +
              "," + std::to_string(pe[2]) + ") sview=(" + std::to_string(vx) + "," +
              std::to_string(vy) + "," + std::to_string(vz) + ") distort=" + std::to_string(d) +
              " uv=(" + std::to_string(u) + "," + std::to_string(v) + ") ndcZ=" +
              std::to_string(nz) + (off ? "  <-- OFF-MAP" : ""));
     }
    }
   }
  }
  // END TEMP DIAGNOSTIC
  return result;
}
} // namespace net::minecraft::client::render::shadowmap
