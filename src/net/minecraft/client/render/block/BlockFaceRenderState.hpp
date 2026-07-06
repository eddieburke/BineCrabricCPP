#pragma once
namespace net::minecraft::client::render::block {
// Per-face vertex colours emitted by AO (or uniform flat lighting).
struct BlockFaceVertexColors {
  float red[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float green[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float blue[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};
struct BlockFaceRenderState {
  bool useAo = false;
  BlockFaceVertexColors colors;
};
} // namespace net::minecraft::client::render::block
