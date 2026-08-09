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
  // The depth size the driver ACTUALLY gave us, queried after allocation — a
  // DEPTH_COMPONENT32 request is commonly satisfied with 24. glPolygonOffset units
  // are multiples of the smallest resolvable increment of this, so a bias expressed
  // in blocks is meaningless without it.
  int shadowDepthBits = 24;
  // https://shaders.properties/current/reference/buffers/shadowtex/
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  bool depthCompare = false;
  // see docs/agent-notes/shader-system-rebuild.md:106 Stage F: skip the redundant
  // addColorAttachment/drawBuffers pair in attachCompositeColors when flip parity
  // did not change since the last call for this fbo.
  std::array<unsigned int, 8> compositeAttach{};
  int compositeDrawBufferCount = -1;

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
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
  ShadowCullingFrustum terrainFrustum{};
  ShadowCullingFrustum entityFrustum{};
};

// The single definition of the shadow camera: a pure function of the pack directives,
// the celestial angle and THIS frame's player camera. Both the shadow map render and
// the shadowModelView/shadowProjection uniforms call this, so they cannot disagree.
// Java derives the uniforms the same way — on demand from the live camera position
// (MatrixUniforms.addShadowMatrix -> ShadowRenderer.createShadowModelView ->
// CameraUniforms.getUnshiftedCameraPosition) — rather than reading back a camera the
// shadow pass left behind, which is what let the matrices lag a frame here.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/MatrixUniforms.java
FrameRenderCamera makeShadowCamera(const PackDefinition& definition,
                                   const FrameRenderCamera& camera,
                                   const CelestialState& celestial);

void reset(ShadowMapState& state);
void snapshotOpaqueDepth(ShadowMapState& state);
ShadowMapResult update(ShadowMapState& state,
                       GameRenderer& renderer,
                       float tickDelta,
                       const FrameRenderCamera& camera,
                       const PackDefinition& definition);
} // namespace shadowmap
} // namespace net::minecraft::client::render
