#pragma once
#include <vector>
#include "net/minecraft/client/model/Quad.hpp"
#include "net/minecraft/client/model/Vertex.hpp"
namespace net::minecraft::client::model {
class ModelPart {
public:
  explicit ModelPart(int textureU = 0, int textureV = 0) : textureU(textureU), textureV(textureV) {
  }
  ModelPart(const ModelPart& other);
  ModelPart& operator=(const ModelPart& other);
  ModelPart(ModelPart&& other) noexcept = default;
  ModelPart& operator=(ModelPart&& other) noexcept = default;
  void addCuboid(float x, float y, float z, int width, int height, int depth, float dilation = 0.0f);
  void clearCuboids();
  void setPivot(float x, float y, float z);
  void render(float scale);
  void renderForceTransform(float scale);
  void transform(float scale);
  void addChild(ModelPart& child);
  int textureU = 0;
  int textureV = 0;
  bool mirror = false;
  bool visible = true;
  bool hidden = false;
  float pitch = 0.0f;
  float yaw = 0.0f;
  float roll = 0.0f;
  float pivotX = 0.0f;
  float pivotY = 0.0f;
  float pivotZ = 0.0f;

private:
  void renderFaces(float scale) const;
  std::vector<Quad> faces_;
  std::vector<ModelPart*> children_;
};
} // namespace net::minecraft::client::model
