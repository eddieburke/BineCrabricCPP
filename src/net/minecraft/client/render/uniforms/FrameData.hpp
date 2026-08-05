#pragma once
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::client::render {
struct FrameRenderCamera;
PackUniformValues buildShaderFrameData(int width, int height, float worldTime,
                                         int shadowMapResolution, bool normalAvailable, bool shadowAvailable,
                                         const FrameRenderCamera& camera,
                                         const FrameRenderCamera& shadowCamera,
                                         const net::minecraft::World* world,
                                         float eyeBrightnessHalflife = 10.0f);
float smoothExponential(float target, float& accumulator, bool& initialized, float frameTime,
                        float halfLifeSeconds);
[[nodiscard]] float updateCenterDepthSmooth(float windowDepth01, float nearPlane, float farPlane, float frameTime,
                                            float halfLife);
[[nodiscard]] float updateWetnessSmooth(float rainStrength, float frameTime, float wetnessHalflife,
                                        float drynessHalflife);
} // namespace net::minecraft::client::render
