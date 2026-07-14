#pragma once
#include <string>
#include <vector>
namespace net::minecraft::mod::model {
// Face order matches vanilla Direction ordinals: down, up, north, south, west, east.
enum class ModelFace {
  Down = 0,
  Up,
  North,
  South,
  West,
  East
};
inline constexpr int kModelFaceCount = 6;
struct BakedVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  // Normalized texture coordinates (0..1 across the face's texture).
  float u = 0.0f;
  float v = 0.0f;
};
struct BakedQuad {
  ModelFace face = ModelFace::Down;
  // Vanilla directional shading factor (1.0 when the element disables shade).
  float shade = 1.0f;
  int tintIndex = -1;
  float red = 1.0f;
  float green = 1.0f;
  float blue = 1.0f;
  float alpha = 1.0f;
  BakedVertex vertices[4];
};
// Quads grouped by resolved texture path so draws bind each texture once.
// An empty texturePath marks an untextured batch colored per quad.
struct BakedTextureBatch {
  std::string texturePath;
  std::vector<BakedQuad> quads;
};
// Axis-aligned bounds over all baked vertices, in model space (0..1 units).
struct BakedBounds {
  float min[3] = {0.0f, 0.0f, 0.0f};
  float max[3] = {0.0f, 0.0f, 0.0f};
  bool empty = true;
};
struct BakedModel {
  std::vector<BakedTextureBatch> batches;
  BakedBounds bounds;
};
// Recomputes bounds from the current batches; call after building quads.
void computeBakedBounds(BakedModel& model);
} // namespace net::minecraft::mod::model
