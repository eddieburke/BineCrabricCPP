#pragma once
#include <algorithm>
#include <cmath>
namespace net::minecraft::client::option {
struct RenderDistance {
 float blocks = 256.0f;
 // The 0..3 options step this was derived from. Kept because the fog colour
 // blend is a function of the step, not of the distance.
 int setting = 0;
 [[nodiscard]] int chunks() const noexcept {
  return std::clamp(static_cast<int>((blocks + 8.0f) / 16.0f), kMinChunks, kMaxChunks);
 }
 [[nodiscard]] float sectionCoverageBlocks() const noexcept {
  return static_cast<float>(chunks() * 16);
 }
 // Vanilla's near plane (Java GameRenderer.getBasicProjectionMatrix). Not a
 // function of the distance, but it is the other half of every projection built
 // here, and it was written out separately at each one.
 static constexpr float kNearPlane = 0.05f;
 [[nodiscard]] float nearPlane() const noexcept {
  return kNearPlane;
 }
 [[nodiscard]] float farPlane() const noexcept {
  return blocks * 2.0f;
 }
 // Beta EntityRenderer.setupFog: GL_FOG_START = farPlaneDistance * 0.25, GL_FOG_END
 // = farPlaneDistance. NOT modern Java's 0.75 — Iris only captures whatever the game
 // set (FogUniforms.java reads sodium$getFogParameters()), it defines no ratio, and
 // every other term here (farPlane * 2, the 1/(4 - setting) colour blend, the four
 // distance steps) is Beta's. At 0.75 the whole fade is crushed into the last quarter
 // of the view distance, which reads as a wall rather than a haze.
 static constexpr float kFogStartRatio = 0.25f;
 [[nodiscard]] float fogEnd() const noexcept {
  return blocks;
 }
 [[nodiscard]] float fogStart() const noexcept {
  return blocks * kFogStartRatio;
 }
 // The sky pass (Beta setupFog(-1)): start 0, end farPlaneDistance * 0.8, so the sky
 // itself blends toward the fog colour instead of meeting fogged terrain unblended.
 static constexpr float kSkyFogEndRatio = 0.8f;
 [[nodiscard]] float skyFogEnd() const noexcept {
  return blocks * kSkyFogEndRatio;
 }
 // How far the horizon fog pulls toward the sky colour. A function of the
 // options step, not the block distance: shorter view distances wash out more.
 [[nodiscard]] float fogColorBlend() const noexcept {
  const float blend = 1.0f / static_cast<float>(4 - setting);
  return 1.0f - static_cast<float>(std::pow(static_cast<double>(blend), 0.25));
 }
 [[nodiscard]] bool operator==(const RenderDistance&) const noexcept = default;
 static constexpr int kMinChunks = 2;
 static constexpr int kMaxChunks = 80;
};
} // namespace net::minecraft::client::option
