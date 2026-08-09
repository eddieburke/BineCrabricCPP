#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "net/minecraft/client/model/Vertex.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::model {
class Quad {
 public:
 std::vector<Vertex> vertices;
 int verticesCount = 0;
 bool flipNormal = false;
 Quad() = default;
 explicit Quad(std::vector<Vertex> verticesIn)
     : vertices(std::move(verticesIn)), verticesCount(static_cast<int>(vertices.size())) {
  updateNormal();
 }
 explicit Quad(const std::array<Vertex, 4>& verticesIn)
     : vertices(verticesIn.begin(), verticesIn.end()), verticesCount(4) {
  updateNormal();
 }
 Quad(std::vector<Vertex> verticesIn, int u1, int v1, int u2, int v2) : Quad(std::move(verticesIn)) {
  remap(u1, v1, u2, v2);
 }
 Quad(const std::array<Vertex, 4>& verticesIn, int u1, int v1, int u2, int v2) : Quad(verticesIn) {
  remap(u1, v1, u2, v2);
 }
 void flip() {
  std::reverse(vertices.begin(), vertices.end());
  updateNormal();
 }
 void emitTo(render::Tessellator& tessellator, float scale) const {
  if(vertices.size() < 3) {
   return;
  }
  const float sign = flipNormal ? -1.0f : 1.0f;
  tessellator.normal(normal_[0] * sign, normal_[1] * sign, normal_[2] * sign);
  for(const Vertex& vertex : vertices) {
   tessellator.vertex(vertex.pos.x * scale, vertex.pos.y * scale, vertex.pos.z * scale, vertex.u, vertex.v);
  }
 }
 void render(render::Tessellator& tessellator, float scale) const {
  if(vertices.size() < 3) {
   return;
  }
  tessellator.startQuads();
  emitTo(tessellator, scale);
  tessellator.draw();
 }

 private:
 void remap(int u1, int v1, int u2, int v2) {
  if(vertices.size() < 4) {
   return;
  }
  constexpr float uvInsetU = 0.0015625f;
  constexpr float uvInsetV = 0.003125f;
  vertices[0] =
      vertices[0].remap(static_cast<float>(u2) / 64.0f - uvInsetU, static_cast<float>(v1) / 32.0f + uvInsetV);
  vertices[1] =
      vertices[1].remap(static_cast<float>(u1) / 64.0f + uvInsetU, static_cast<float>(v1) / 32.0f + uvInsetV);
  vertices[2] =
      vertices[2].remap(static_cast<float>(u1) / 64.0f + uvInsetU, static_cast<float>(v2) / 32.0f - uvInsetV);
  vertices[3] =
      vertices[3].remap(static_cast<float>(u2) / 64.0f - uvInsetU, static_cast<float>(v2) / 32.0f - uvInsetV);
 }
 void updateNormal() {
  if(vertices.size() < 3) {
   normal_ = {0.0f, 1.0f, 0.0f};
   return;
  }
  const Vec3d edgeA = vertices[0].pos.relativize(vertices[1].pos);
  const Vec3d edgeB = vertices[2].pos.relativize(vertices[1].pos);
  const Vec3d normal = edgeB.crossProduct(edgeA);
  const double length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
  if(length <= 1.0e-12) {
   normal_ = {0.0f, 1.0f, 0.0f};
   return;
  }
  const double inv = 1.0 / length;
  normal_ = {static_cast<float>(normal.x * inv), static_cast<float>(normal.y * inv), static_cast<float>(normal.z * inv)};
 }
 std::array<float, 3> normal_ = {0.0f, 1.0f, 0.0f};
};
} // namespace net::minecraft::client::model
