#pragma once
#include <array>
#include "net/minecraft/client/gl/GlFramebuffer.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/culling/ShadowFrustum.hpp"
namespace net::minecraft::client::render {
class GameRenderer;
struct PackDefinition;
namespace shadowmap {
struct ShadowTargets {
  gl::GlFramebuffer fbo;
  unsigned int shadowtex0 = 0; // depth attachment (all geometry)
  unsigned int shadowtex1 = 0; // opaque-only copy (no translucent)
  int shadowtex1Resolution = 0;
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderTargets.java
  std::array<std::array<unsigned int, 2>, 8> shadowcolor{};
  int colorCount = 0;
  int resolution = 0;
  // https://shaders.properties/current/reference/buffers/shadowtex/
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  bool depthCompare = false;

  bool ensure(int resolutionIn, int colorBuffers);
  void destroy();
  void snapshotOpaqueDepth(); // copy shadowtex0 → shadowtex1 after opaque draws
  void attachCompositeColors(const std::array<bool, 8>& flipped, int colorBuffers);
  void prepareForShadowRender();
  [[nodiscard]] bool valid() const noexcept { return fbo.valid() && shadowtex0 != 0 && resolution > 0; }
};

struct ShadowMapResult {
  int depthTexture = -1;
  int opaqueDepthTexture = -1;
  int resolution = 0;
  std::array<int, 8> colorTextures = {-1, -1, -1, -1, -1, -1, -1, -1};
  std::array<int, 8> colorAltTextures = {-1, -1, -1, -1, -1, -1, -1, -1};
  int colorCount = 0;
};

struct ShadowMapState {
  ShadowTargets targets{};
  FrameRenderCamera shadowCamera{};
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  ShadowCullingFrustum terrainFrustum{};
  ShadowCullingFrustum entityFrustum{};
};

void reset(ShadowMapState& state);
void snapshotOpaqueDepth(ShadowMapState& state);
ShadowMapResult update(ShadowMapState& state,
                       GameRenderer& renderer,
                       float tickDelta,
                       const FrameRenderCamera& camera,
                       const PackDefinition& definition);
} // namespace shadowmap
} // namespace net::minecraft::client::render
