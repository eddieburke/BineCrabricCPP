#pragma once
#include <algorithm>
#include <cmath>
namespace net::minecraft::client::option {
// "How far do we render", as one value instead of thirteen names.
//
 // Blocks is the truth (the camera far plane, the fog end and the shader's
 // renderDistance uniform are all in blocks); the section radius is DERIVED, never
 // stored alongside. Storing both is what let a consumer hold a stale half-width.
struct RenderDistance {
 float blocks = 256.0f;
 // The 0..3 options step this was derived from. Kept because the fog colour
 // blend is a function of the step, not of the distance.
 int setting = 0;
 // Section radius in chunks. The +8 rounds a half-chunk up so the ring covers the
 // block distance rather than falling one section short of it.
 [[nodiscard]] int chunks() const noexcept {
  return std::clamp(static_cast<int>((blocks + 8.0f) / 16.0f), kMinChunks, kMaxChunks);
 }
 // The block distance the section ring actually reaches, which is not `blocks`
 // once the clamp bites. Culling distances must use this, not `blocks`, or the
 // frustum keeps asking for sections the ring never creates.
 [[nodiscard]] float ringBlocks() const noexcept {
  return static_cast<float>(chunks() * 16);
 }
 // --- Everything below is derived from the same distance, in one place. ---
 //
 // These were scattered as literals and open-coded ratios: the near plane as a
 // bare 0.05f at each projection site, the fog span as `blocks` and
 // `blocks * 0.75` inside RenderCore, the colour blend as a pow() in
 // RenderSettings. Each is a rule about how far you can see, so each belongs
 // with the distance rather than at whichever call site needed it.

 // Vanilla's near plane (Java GameRenderer.getBasicProjectionMatrix). Not a
 // function of the distance, but it is the other half of every projection built
 // here, and it was written out separately at each one.
 static constexpr float kNearPlane = 0.05f;
 [[nodiscard]] float nearPlane() const noexcept {
  return kNearPlane;
 }
 [[nodiscard]] float farPlane() const noexcept {
  return blocks;
 }
 // Iris 26.1 (FogRenderer.setupFog -> FogStorage, FogUniforms.java): terrain fog
 // runs from 0.75*distance to the far plane exactly, so a pack's vanilla_fog()
 // blends against the real distance. Callers scale these by their own mod
 // start/end factors; the ratio itself lives here.
 static constexpr float kFogStartRatio = 0.75f;
 [[nodiscard]] float fogEnd() const noexcept {
  return blocks;
 }
 [[nodiscard]] float fogStart() const noexcept {
  return blocks * kFogStartRatio;
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
