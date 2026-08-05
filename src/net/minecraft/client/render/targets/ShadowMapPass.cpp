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
  // DEPTH_COMPONENT32 is a REQUEST. Drivers routinely hand back 24 bits (it is the
  // only depth size some of them keep in hardware), and glPolygonOffset units are
  // multiples of the smallest resolvable increment of whatever we actually got. A
  // bias stated in blocks therefore has to be converted with the real depth size,
  // not the one we asked for — assuming 32 when the buffer is 24 overshoots by 256x
  // and peter-pans every shadow off its caster.
  // see src/net/minecraft/client/render/GameRenderer.cpp shadowDepthBias
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
FrameRenderCamera makeShadowCamera(const PackDefinition& definition,
                                   const FrameRenderCamera& camera,
                                   const CelestialState& celestial) {
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
  const bool perspectiveShadow = definition.shadowMapFov > 0.0f;
  FrameRenderCamera shadowCam{};
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
  shadowCam.x = camera.eyeX;
  shadowCam.y = camera.eyeY;
  shadowCam.z = camera.eyeZ;
  shadowCam.eyeX = camera.eyeX;
  shadowCam.eyeY = camera.eyeY;
  shadowCam.eyeZ = camera.eyeZ;
  shadowCam.customView = true;
  shadowCam.hideFirstPersonHand = true;
  shadowCam.orthographic = !perspectiveShadow;
  // Iris: the shadow MAP always covers the pack's raw shadowDistance
  // (ShadowRenderer.renderShadows -> createOrthoMatrix(halfPlaneLength, ...),
  // ShadowMatrices.java), while shadowDistanceRenderMul scales ONLY the culling
  // frustum (createShadowFrustum -> halfPlaneLength * renderMultiplier). Feeding the
  // effective distance (mul * renderDistanceBlocks) into the ortho half width AND
  // the frustum made the map collapse with the beta render distance (32-256 blocks)
  // for packs that set shadowDistanceRenderMul (SEUS PTGI sets 1.0), leaving shadows
  // a chunk in front of the player.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  shadowCam.orthoHalfWidth = definition.shadowDistance;
  shadowCam.orthoHalfHeight = definition.shadowDistance;
  shadowCam.orthoNear = definition.shadowNearPlane;
  shadowCam.orthoFar = definition.shadowFarPlane;
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
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
   shadowCam.perspectiveNear = std::max(0.05f, definition.shadowNearPlane);
   shadowCam.perspectiveFar = 156.0f;
   // createPerspectiveMatrix puts the same yScale on BOTH axes — the shadow map is
   // square, so there is no aspect divide. Stating it here means the projection
   // uniform no longer has to be read back out of the pass that rendered it.
   const float yScale = 1.0f / std::tan(definition.shadowMapFov * 3.14159265f / 360.0f);
   shadowCam.projectionX = yScale;
   shadowCam.projectionY = yScale;
  }
  return shadowCam;
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
  // "vanilla". The camera-shaped values (planes, coverage, snap, celestial angle, the
  // per-kind shadow toggles) live in makeShadowCamera; what stays here is what only the
  // render and the culling frusta need.
  const int requestedResolution = definition.shadowMapResolution;
  const int colorBuffers = std::clamp(definition.shadowColorBuffers, 0, 8);
  const ShadowCullState cullState = definition.shadowCulling;
  const float voxelDistance = definition.voxelDistance;
  const float shadowDistance = definition.shadowDistance;
  const float shadowDistanceRenderMul = definition.shadowDistanceRenderMul;
  const float entityDistanceMultiplier = definition.entityShadowDistanceMul;
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
  const CelestialState& celestial = core::celestialState();
  const double centerX = camera.eyeX;
  const double centerY = camera.eyeY;
  const double centerZ = camera.eyeZ;
  FrameRenderCamera shadowCam = makeShadowCamera(definition, camera, celestial);
  // The depth size the driver ACTUALLY gave us (queried at allocation) only exists on
  // the state; the pure camera builder cannot know it. GameRenderer's polygon-offset
  // bias reads camera.shadowDepthBits, so it must be stamped in here, not defaulted.
  // see src/net/minecraft/client/render/GameRenderer.cpp shadowDepthBias
  shadowCam.shadowDepthBits = state.targets.shadowDepthBits;
  // ShadowRenderer passes getShadowLightPositionInWorldSpace() normalised, which is
  // exactly CelestialState.shadowLightDirectionWorld. Reading the model view's column 2
  // ([8][9][10]) picked a perpendicular basis vector, so the extruded cull frustum
  // pointed the wrong way.
  // see src/net/minecraft/client/render/celestial/CelestialState.hpp
  float lightVector[3] = {celestial.shadowLightDirectionWorld[0], celestial.shadowLightDirectionWorld[1],
                          celestial.shadowLightDirectionWorld[2]};
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  float playerModelViewProjection[16]{};
  {
   float playerProjection[16]{};
   float playerModelView[16]{};
   // The player camera carries the render-distance far plane (GameRenderer set it at
   // the start of this frame's renderWorld), so no far plane is passed in.
   buildCameraProjection(playerProjection, camera);
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
  // TEMP DIAGNOSTIC — [shadow-probe]. Restored after 4dd55608 deleted it; without this
  // an empty shadow map and a misoriented one look identical on screen, which is how
  // this bug kept getting "fixed" blind. DELETE once shadows are correct.
  {
   static int shadowProbeFrame = 0;
   if((shadowProbeFrame++ % 120) == 0) {
    // 1. How far out does the map actually have content? A centre patch cannot answer
    // this: packs concentrate texels near the camera (RenderPearl's squircle puts ~10x
    // magnification at the centre), so the middle 32x32 texels span a fraction of a
    // block and always look "written". Read the centre row and column instead and
    // report the outermost written texel as a fraction of the map's half-width — that
    // is the shadow map's reach in its own NDC, which is what the pack samples against.
    gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, state.targets.fbo.id());
    std::vector<float> row(static_cast<std::size_t>(resolution), 1.0f);
    std::vector<float> column(static_cast<std::size_t>(resolution), 1.0f);
    ::glReadPixels(0, resolution / 2, resolution, 1, gl::pixel::DepthComponent, 0x1406, row.data());
    ::glReadPixels(resolution / 2, 0, 1, resolution, gl::pixel::DepthComponent, 0x1406, column.data());
    gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, 0);
    const auto scanAxis = [resolution](const std::vector<float>& samples, int& written,
                                       double& outermostFraction) {
     written = 0;
     outermostFraction = 0.0;
     const double centre = resolution * 0.5;
     for(std::size_t i = 0; i < samples.size(); ++i) {
      if(samples[i] >= 0.999f) continue;
      ++written;
      outermostFraction = std::max(outermostFraction, std::abs(static_cast<double>(i) - centre) / centre);
     }
    };
    int rowWritten = 0;
    int columnWritten = 0;
    double rowReach = 0.0;
    double columnReach = 0.0;
    scanAxis(row, rowWritten, rowReach);
    scanAxis(column, columnWritten, columnReach);
    float minDepth = 1.0f;
    float maxDepth = 0.0f;
    for(const float d : row) {
     if(d >= 0.999f) continue;
     minDepth = std::min(minDepth, d);
     maxDepth = std::max(maxDepth, d);
    }
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[shadow-probe] map res=") + std::to_string(resolution) + " rowWritten=" +
            std::to_string(rowWritten) + "/" + std::to_string(resolution) + " colWritten=" +
            std::to_string(columnWritten) + "/" + std::to_string(resolution) + " reach=" +
            std::to_string(std::max(rowReach, columnReach)) + " of half-width" + " depth min=" +
            std::to_string(minDepth) + " max=" + std::to_string(maxDepth) +
            (rowWritten == 0 && columnWritten == 0 ? "  <-- MAP IS EMPTY" : ""));
    // 2. The directives the engine resolved, so they can be diffed against the pack's
    // own consts.
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[shadow-probe] shadowDistance=") + std::to_string(shadowDistance) +
            " renderMul=" + std::to_string(shadowDistanceRenderMul) + " orthoHalf=" +
            std::to_string(shadowCam.orthoHalfWidth) + " near=" + std::to_string(shadowCam.orthoNear) +
            " far=" + std::to_string(shadowCam.orthoFar) + " intervalSize=" +
            std::to_string(definition.shadowIntervalSize) + " sunPathRotation=" +
            std::to_string(celestial.sunPathRotation) + " fov=" + std::to_string(definition.shadowMapFov) +
            " ortho=" + std::to_string(static_cast<int>(shadowCam.orthographic)));
    // 2b. Cull volume vs map volume. The map covers orthoHalfWidth, but the terrain
    // frustum only admits casters within halfPlaneLength * renderMultiplier. If the
    // cull distance is the smaller of the two, the outer ring of the map is
    // permanently empty and shadows stop short of where the map claims to reach.
    const float terrainCull = shadowDistance * (shadowDistanceRenderMul >= 0.0f ? shadowDistanceRenderMul : 1.0f);
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[shadow-probe] cull terrainDistance=") + std::to_string(terrainCull) +
            " mapCovers=" + std::to_string(shadowCam.orthoHalfWidth) + " entityMul=" +
            std::to_string(entityDistanceMultiplier) + " voxelDistance=" + std::to_string(voxelDistance) +
            " cullState=" + std::to_string(static_cast<int>(cullState)) +
            (terrainCull < shadowCam.orthoHalfWidth - 0.5f
                 ? "  <-- OUTER RING OF MAP HAS NO CASTERS"
                 : ""));
    // 3. THE test that matters: the shadow map's own toward-light axis (model view
    // column 2) against the shadowLightPosition direction the pack lights with. These
    // must be parallel — dot ~ 1.0. Anything else and the pack samples the map from a
    // direction the map was never rendered from, which reads on screen as "no shadows".
    const float* m = shadowCam.explicitModelView;
    const float axis[3] = {m[2], m[6], m[10]};
    const float dot = axis[0] * lightVector[0] + axis[1] * lightVector[1] + axis[2] * lightVector[2];
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[shadow-probe] mapAxis=(") + std::to_string(axis[0]) + "," +
            std::to_string(axis[1]) + "," + std::to_string(axis[2]) + ") lightVec=(" +
            std::to_string(lightVector[0]) + "," + std::to_string(lightVector[1]) + "," +
            std::to_string(lightVector[2]) + ") dot=" + std::to_string(dot) +
            (std::abs(dot - 1.0f) > 0.01f ? "  <-- LIGHT AXIS MISALIGNED (expected ~1.0)" : "  ok") +
            " celestialAngle=" + std::to_string(celestial.celestialAngle) + " shadowAngle=" +
            std::to_string(celestial.shadowAngle) + " day=" + std::to_string(static_cast<int>(celestial.day)));
    // 4. GPU truth, read back from the driver instead of our own assumptions: the
    // depth attachment's real resolution, the depth texture's internal format, the
    // compare state the hardware sampler applies, and — the pack-vs-engine ortho
    // contract — the engine's uploaded shadowProjection terms against the values the
    // pack's compile-time shadow_proj_scale (1/shadowDistance, -2/(far-near)) expects.
    // The map is rendered by shadow.vsh writing its own clip position, so a mismatch
    // here does not corrupt the map itself, but it does break every other consumer of
    // the shadowProjection uniform and shows up as acne or misplaced shadows.
    {
     core::bindTexture(gl::cap::Texture2D, static_cast<int>(state.targets.shadowtex0));
     int texInternalFormat = 0;
     int texDepthBits = 0;
     ::glGetTexLevelParameteriv(gl::cap::Texture2D, 0, 0x1001, &texInternalFormat);
     ::glGetTexLevelParameteriv(gl::cap::Texture2D, 0, static_cast<unsigned>(kTextureDepthSize), &texDepthBits);
     int compareMode = 0;
     int compareFunc = 0;
     ::glGetTexParameteriv(gl::cap::Texture2D, static_cast<unsigned>(kTextureCompareMode), &compareMode);
     ::glGetTexParameteriv(gl::cap::Texture2D, static_cast<unsigned>(kTextureCompareFunc), &compareFunc);
     float shadowOrtho[16]{};
     buildCameraProjection(shadowOrtho, shadowCam);
     const float expectedX = 1.0f / std::max(shadowDistance, 1e-3f);
     const float expectedZ = -2.0f / std::max(definition.shadowFarPlane - definition.shadowNearPlane, 1e-3f);
     const float expectedW = -(definition.shadowFarPlane + definition.shadowNearPlane) /
                             std::max(definition.shadowFarPlane - definition.shadowNearPlane, 1e-3f);
     ::net::minecraft::client::ClientLog::LOGGER.log(
         ::net::minecraft::util::logging::LogLevel::Info,
         std::string("[shadow-probe] GPU texDepthBits=") + std::to_string(texDepthBits) +
             " internalFormat=0x" + std::to_string(texInternalFormat) + " compareMode=0x" +
             std::to_string(compareMode) + " compareFunc=0x" + std::to_string(compareFunc) +
             " orthoContract m0=" + std::to_string(shadowOrtho[0]) + " vs pack " +
             std::to_string(expectedX) + " m10=" + std::to_string(shadowOrtho[10]) + " vs pack " +
             std::to_string(expectedZ) + " m14=" + std::to_string(shadowOrtho[14]) + " vs pack " +
             std::to_string(expectedW));
    }
   }
  }
  return result;
 }
} // namespace net::minecraft::client::render::shadowmap
