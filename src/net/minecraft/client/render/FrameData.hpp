#pragma once
#include "net/minecraft/client/render/shaderpack/ShaderUniforms.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::client::render {
struct FrameRenderCamera;
}
namespace net::minecraft::client::render::shaderpack {
ShaderUniformValues buildShaderFrameData(int width, int height, float farPlane, float worldTime,
                                         int shadowMapResolution, bool normalAvailable, bool shadowAvailable,
                                         const render::FrameRenderCamera& camera,
                                         const render::FrameRenderCamera& shadowCamera,
                                         const net::minecraft::World* world,
                                         float eyeBrightnessHalflifeTicks = 10.0f);
[[nodiscard]] float updateCenterDepthSmooth(float windowDepth01, float nearPlane, float farPlane, float frameTime,
                                            float halfLifeSeconds);
[[nodiscard]] float updateWetnessSmooth(float rainStrength, float frameTime, float wetnessHalflife,
                                        float drynessHalflife);
} // namespace net::minecraft::client::render::shaderpack
