#pragma once
#include <array>
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
namespace net::minecraft::client::render {
class GameRenderer;
namespace shaderpack {
struct ShaderPackDefinition;
}
namespace shadowmap {
struct ShadowTargets {
 unsigned int fbo = 0;
 unsigned int shadowtex0 = 0; // depth attachment (all geometry)
 unsigned int shadowtex1 = 0; // opaque-only copy (no translucent)
 int shadowtex1Resolution = 0;
 std::array<unsigned int, 8> shadowcolor{};
 int colorCount = 0;
 int resolution = 0;
 // GL_TEXTURE_COMPARE for sampler2DShadow / *HW when packs enable hardware filtering.
 bool depthCompare = true;

 bool ensure(int resolutionIn, int colorBuffers);
 void destroy();
 void bind() const;
 void snapshotOpaqueDepth(); // copy shadowtex0 → shadowtex1 after opaque draws
 [[nodiscard]] bool valid() const noexcept { return fbo != 0 && shadowtex0 != 0 && resolution > 0; }
};

struct ShadowMapResult {
 int depthTexture = -1;
 int opaqueDepthTexture = -1;
 int resolution = 0;
 std::array<int, 8> colorTextures = {-1, -1, -1, -1, -1, -1, -1, -1};
 int colorCount = 0;
};

struct ShadowMapState {
 ShadowTargets targets{};
 FrameRenderCamera shadowCamera{};
};

void reset(ShadowMapState& state);
void snapshotOpaqueDepth(ShadowMapState& state);
ShadowMapResult update(ShadowMapState& state,
                       GameRenderer& renderer,
                       float tickDelta,
                       const FrameRenderCamera& camera,
                       float farPlane,
                       const shaderpack::ShaderPackDefinition* definition);
} // namespace shadowmap
} // namespace net::minecraft::client::render
