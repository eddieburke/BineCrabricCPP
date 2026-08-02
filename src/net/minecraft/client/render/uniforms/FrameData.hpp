#pragma once
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::client::render {
struct FrameRenderCamera;
PackUniformValues buildShaderFrameData(int width, int height, float farPlane, float worldTime,
                                         int shadowMapResolution, bool normalAvailable, bool shadowAvailable,
                                         const FrameRenderCamera& camera,
                                         const FrameRenderCamera& shadowCamera,
                                         const net::minecraft::World* world,
                                         float eyeBrightnessHalflife = 10.0f);
// Java SmoothedFloat (SmoothedFloat.java): exponential smoothing with the given half life in
// seconds; the first call seeds the accumulator with the raw value instead of smoothing.
// State lives in the by-ref accumulator; the return value is a convenience copy, so the
// function is intentionally not [[nodiscard]] (callers that only need the accumulator
// advance may ignore it).
float smoothExponential(float target, float& accumulator, bool& initialized, float frameTime,
                        float halfLifeSeconds);
// Half-life parameters below follow the Java directive convention: units of deciseconds
// (1/10 s, i.e. 2 ticks), converted to seconds with *0.1 exactly like SmoothedFloat and
// CenterDepthSampler do.
[[nodiscard]] float updateCenterDepthSmooth(float windowDepth01, float nearPlane, float farPlane, float frameTime,
                                            float halfLife);
[[nodiscard]] float updateWetnessSmooth(float rainStrength, float frameTime, float wetnessHalflife,
                                        float drynessHalflife);
} // namespace net::minecraft::client::render
