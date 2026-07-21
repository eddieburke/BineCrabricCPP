#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include "net/minecraft/client/gl/GLCore.hpp"
namespace net::minecraft::client::render {
template <typename TVertex>
class BufferBuilder {
 public:
 struct VertexProxy {
  BufferBuilder& builder;
  TVertex* ptr;
  VertexProxy& color(std::uint32_t argb) {
   if constexpr(requires { ptr->color; }) {
    ptr->color = argb;
   }
   return *this;
  }
  VertexProxy& color(float r, float g, float b, float a) {
   if constexpr(requires { ptr->color; }) {
    auto r_byte = static_cast<std::uint32_t>(r * 255.0f) & 0xFF;
    auto g_byte = static_cast<std::uint32_t>(g * 255.0f) & 0xFF;
    auto b_byte = static_cast<std::uint32_t>(b * 255.0f) & 0xFF;
    auto a_byte = static_cast<std::uint32_t>(a * 255.0f) & 0xFF;
    ptr->color = r_byte | (g_byte << 8) | (b_byte << 16) | (a_byte << 24);
   }
   return *this;
  }
  VertexProxy& color(int r, int g, int b, int a) {
   if constexpr(requires { ptr->color; }) {
    ptr->color = (r & 0xFF) | ((g & 0xFF) << 8) | ((b & 0xFF) << 16) | ((a & 0xFF) << 24);
   }
   return *this;
  }
  VertexProxy& tex(float u, float v) {
   if constexpr(requires { ptr->u; }) {
    ptr->u = u;
    ptr->v = v;
   }
   return *this;
  }
  VertexProxy& normal(float x, float y, float z) {
   if constexpr(requires { ptr->normal; }) {
    auto nx = static_cast<std::int32_t>(x * 127.0f) & 0xFF;
    auto ny = static_cast<std::int32_t>(y * 127.0f) & 0xFF;
    auto nz = static_cast<std::int32_t>(z * 127.0f) & 0xFF;
    ptr->normal = nx | (ny << 8) | (nz << 16);
   }
   return *this;
  }
  VertexProxy& light(std::int16_t block, std::int16_t sky) {
   if constexpr(requires { ptr->light; }) {
    ptr->light = static_cast<std::int16_t>((block & 0xFFFF) | ((sky & 0xFFFF) << 16));
   }
   return *this;
  }
  void next() {
   builder.nextVertex();
  }
 };
 BufferBuilder(std::size_t initialCapacity = 2097152) {
  buffer_.reserve(initialCapacity);
 }
 void begin(int drawMode) {
  drawMode_ = drawMode;
  vertexCount_ = 0;
  buffer_.clear();
 }
 VertexProxy vertex(float x, float y, float z) {
  std::size_t offset = buffer_.size();
  buffer_.resize(offset + sizeof(TVertex));
  TVertex* ptr = reinterpret_cast<TVertex*>(&buffer_[offset]);
  ptr->x = x;
  ptr->y = y;
  ptr->z = z;
  if constexpr(requires { ptr->color; }) {
   ptr->color = 0xFFFFFFFFU;
  }
  return VertexProxy{*this, ptr};
 }
 void nextVertex() {
  vertexCount_++;
 }
 void end() {
  // optional validation/completion
 }
 [[nodiscard]] std::size_t vertexCount() const noexcept {
  return vertexCount_;
 }
 [[nodiscard]] int drawMode() const noexcept {
  return drawMode_;
 }
 [[nodiscard]] const std::vector<std::uint8_t>& buffer() const noexcept {
  return buffer_;
 }
 [[nodiscard]] std::vector<std::uint8_t>& buffer() noexcept {
  return buffer_;
 }
 void reset() {
  vertexCount_ = 0;
  buffer_.clear();
 }
 void draw();
 unsigned uploadToGpu() {
  if(buffer_.empty()) return 0;
  unsigned vbo = 0;
  if(gl::GLCore::genBuffers != nullptr) {
   gl::GLCore::genBuffers(1, &vbo);
   gl::GLCore::bindBuffer(0x8892, vbo); // GL_ARRAY_BUFFER
   gl::GLCore::bufferData(0x8892, buffer_.size(), buffer_.data(), 0x88E8); // GL_DYNAMIC_DRAW
   gl::GLCore::bindBuffer(0x8892, 0);
  }
  return vbo;
 }

 private:
 std::vector<std::uint8_t> buffer_;
 std::size_t vertexCount_ = 0;
 int drawMode_ = 0;
};
} // namespace net::minecraft::client::render
#include "net/minecraft/client/gl/EnginePipeline.hpp"
namespace net::minecraft::client::render {
template <typename TVertex>
inline void BufferBuilder<TVertex>::draw() {
 constexpr bool hasTexture = requires(TVertex v) { v.u; };
 constexpr bool hasColor = requires(TVertex v) { v.color; };
 constexpr bool hasNormals = requires(TVertex v) { v.normal; };
 gl::engine_pipeline::drawInterleaved(buffer_.data(),
                                      vertexCount_,
                                      sizeof(TVertex),
                                      drawMode_,
                                      hasTexture,
                                      hasColor,
                                      hasNormals);
}
} // namespace net::minecraft::client::render
