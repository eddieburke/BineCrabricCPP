#include "net/minecraft/client/render/Tessellator.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
namespace net::minecraft::client::render {
namespace {
static int clamp255(float v) {
  return static_cast<int>(std::clamp(v, 0.0f, 1.0f) * 255.0f);
}
} // namespace
Tessellator Tessellator::INSTANCE{};
Tessellator& INSTANCE = Tessellator::INSTANCE;
Tessellator::Tessellator(const std::size_t bufferSize) {
  flushThreshold_ = std::max<std::size_t>(bufferSize / 8, 256);
  vertices_.reserve(std::min<std::size_t>(flushThreshold_, 4096));
}
void Tessellator::startQuads() {
  start(kGlQuads);
}
void Tessellator::start(const int mode) {
  if(drawing_)
    draw();
  drawing_ = true;
  mode_ = mode;
  hasTexture_ = false;
  hasColor_ = false;
  hasNormals_ = false;
  colorDisabled_ = false;
  addedVertexCount_ = 0;
  reset();
}
int Tessellator::effectiveDrawMode(const int mode) noexcept {
  return mode == kGlQuads && kTriangleMode ? gl::prim::Triangles : mode;
}
void Tessellator::expandQuadToTriangles() {
  if(vertices_.size() < 3) {
    return;
  }
  pushVertex(vertices_[vertices_.size() - 3]);
  pushVertex(vertices_[vertices_.size() - 2]);
}
void Tessellator::texture(const double u, const double v) {
  hasTexture_ = true;
  u_ = static_cast<float>(u);
  v_ = static_cast<float>(v);
}
void Tessellator::color(const float r, const float g, const float b) {
  color(clamp255(r), clamp255(g), clamp255(b));
}
void Tessellator::color(const float r, const float g, const float b, const float a) {
  color(clamp255(r), clamp255(g), clamp255(b), clamp255(a));
}
void Tessellator::color(const int r, const int g, const int b) {
  color(r, g, b, 255);
}
void Tessellator::color(const int r, const int g, const int b, const int a) {
  if(colorDisabled_)
    return;
  hasColor_ = true;
  currentColor_ = (static_cast<std::uint32_t>(std::clamp(a, 0, 255)) << 24U) |
                  (static_cast<std::uint32_t>(std::clamp(b, 0, 255)) << 16U) |
                  (static_cast<std::uint32_t>(std::clamp(g, 0, 255)) << 8U) |
                  static_cast<std::uint32_t>(std::clamp(r, 0, 255));
}
void Tessellator::color(const int rgb) {
  color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}
void Tessellator::color(const int rgb, const int a) {
  color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, a);
}
void Tessellator::disableColor() {
  colorDisabled_ = true;
}
void Tessellator::normal(const float x, const float y, const float z) {
  hasNormals_ = true;
  currentNormal_ =
      static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(x * 127.0f))) |
      (static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(y * 127.0f))) << 8U) |
      (static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(z * 127.0f))) << 16U);
}
void Tessellator::translate(const double x, const double y, const double z) {
  xOffset_ = static_cast<float>(x);
  yOffset_ = static_cast<float>(y);
  zOffset_ = static_cast<float>(z);
}
void Tessellator::translate(const float x, const float y, const float z) {
  xOffset_ += x;
  yOffset_ += y;
  zOffset_ += z;
}
void Tessellator::vertex(const double x, const double y, const double z, const double u, const double v) {
  texture(u, v);
  vertex(x, y, z);
}
void Tessellator::vertex(const double x, const double y, const double z) {
  if(!drawing_)
    return;
  ++addedVertexCount_;
  if(mode_ == kGlQuads && kTriangleMode && addedVertexCount_ % 4 == 0) {
    expandQuadToTriangles();
  }
  TessellatorVertex vx;
  vx.x = static_cast<float>(x) + xOffset_;
  vx.y = static_cast<float>(y) + yOffset_;
  vx.z = static_cast<float>(z) + zOffset_;
  vx.u = hasTexture_ ? u_ : 0.0f;
  vx.v = hasTexture_ ? v_ : 0.0f;
  vx.color = hasColor_ ? currentColor_ : 0xFFFFFFFFU;
  vx.normal = hasNormals_ ? currentNormal_ : 0;
  pushVertex(vx);
  if(vertices_.size() >= flushThreshold_ && vertices_.size() % 4 == 0)
    flush();
}
void Tessellator::draw() {
  if(!drawing_)
    return;
  drawing_ = false;
  if(!vertices_.empty() && !captureOnly_) {
    TessellatorMesh mesh(std::move(vertices_), mode_, hasTexture_, hasColor_, hasNormals_);
    drawMesh(mesh);
    vertices_ = std::move(mesh.vertices);
  }
  reset();
}
void Tessellator::drawMesh(const TessellatorMesh& mesh) {
  if(mesh.vertices.empty()) {
    return;
  }
  gl::GLCore::init();
  const int stride = static_cast<int>(sizeof(TessellatorVertex));
  const int mode = effectiveDrawMode(mesh.mode);
  const bool useVbo = mesh.vbo_ != 0 && gl::GLCore::vboSupported;
  if(useVbo) {
    gl::GLCore::bindBuffer(0x8892, mesh.vbo_);
    gl::engine_pipeline::drawFromBoundBuffer(
        0, mesh.vertices.size(), stride, mode, mesh.hasTexture, mesh.hasColor, mesh.hasNormals);
    gl::GLCore::bindBuffer(0x8892, 0);
  } else {
    gl::engine_pipeline::drawInterleaved(mesh.vertices.data(),
                                         mesh.vertices.size(),
                                         stride,
                                         mode,
                                         mesh.hasTexture,
                                         mesh.hasColor,
                                         mesh.hasNormals);
  }
}
TessellatorMesh Tessellator::takeMesh() {
  TessellatorMesh mesh;
  if(drawing_) {
    drawing_ = false;
    mesh.vertices = std::move(vertices_);
    mesh.mode = mode_;
    mesh.hasTexture = hasTexture_;
    mesh.hasColor = hasColor_;
    mesh.hasNormals = hasNormals_;
  }
  reset();
  return mesh;
}
void Tessellator::pushVertex(const TessellatorVertex& v) {
  vertices_.push_back(v);
}
void Tessellator::flush() {
  if(captureOnly_)
    return;
  const bool was = drawing_;
  draw();
  drawing_ = was;
}
void Tessellator::reset() {
  vertices_.clear();
  addedVertexCount_ = 0;
}
TessellatorMesh::TessellatorMesh(const TessellatorMesh& other)
    : vertices(other.vertices),
      mode(other.mode),
      hasTexture(other.hasTexture),
      hasColor(other.hasColor),
      hasNormals(other.hasNormals) {
}
TessellatorMesh& TessellatorMesh::operator=(const TessellatorMesh& other) {
  if(this != &other) {
    freeGpuBuffer();
    vertices = other.vertices;
    mode = other.mode;
    hasTexture = other.hasTexture;
    hasColor = other.hasColor;
    hasNormals = other.hasNormals;
    vbo_ = 0;
  }
  return *this;
}
TessellatorMesh::TessellatorMesh(TessellatorMesh&& other) noexcept
    : vertices(std::move(other.vertices)),
      mode(other.mode),
      hasTexture(other.hasTexture),
      hasColor(other.hasColor),
      hasNormals(other.hasNormals),
      vbo_(std::exchange(other.vbo_, 0)) {
}
TessellatorMesh& TessellatorMesh::operator=(TessellatorMesh&& other) noexcept {
  if(this != &other) {
    freeGpuBuffer();
    vertices = std::move(other.vertices);
    mode = other.mode;
    hasTexture = other.hasTexture;
    hasColor = other.hasColor;
    hasNormals = other.hasNormals;
    vbo_ = std::exchange(other.vbo_, 0);
  }
  return *this;
}
bool TessellatorMesh::uploadToGpu() {
  gl::GLCore::init();
  if(!gl::GLCore::vboSupported || vertices.empty())
    return false;
  freeGpuBuffer();
  gl::GLCore::genBuffers(1, &vbo_);
  if(!vbo_)
    return false;
  gl::GLCore::bindBuffer(0x8892, vbo_);
  gl::GLCore::bufferData(
      0x8892, static_cast<intptr_t>(vertices.size() * sizeof(TessellatorVertex)), vertices.data(), 0x88E4);
  gl::GLCore::bindBuffer(0x8892, 0);
  return true;
}
void TessellatorMesh::freeGpuBuffer() {
  if(vbo_ && gl::GLCore::deleteBuffers) {
    gl::GLCore::deleteBuffers(1, &vbo_);
    vbo_ = 0;
  }
}
} // namespace net::minecraft::client::render
