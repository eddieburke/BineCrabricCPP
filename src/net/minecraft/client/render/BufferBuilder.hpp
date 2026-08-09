#pragma once
#include <cstdint>
#include <vector>
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
    ptr->light = static_cast<std::int32_t>((block & 0xFFFF) | ((sky & 0xFFFF) << 16));
   }
   return *this;
  }
  void next() {
   builder.nextVertex();
  }
 };
 BufferBuilder(std::size_t initialCapacity = 4096) {
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
  // One copy of a prototype built at compile time, instead of a scattered store
  // per defaulted field on every vertex. Tessellator::vertex is the immediate
  // mode entry point for entities, particles and the GUI, so this runs tens of
  // thousands of times a frame.
  *ptr = kPrototype;
  ptr->x = x;
  ptr->y = y;
  ptr->z = z;
  return VertexProxy{*this, ptr};
 }
 void nextVertex() {
  vertexCount_++;
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

 private:
 // The state every fresh vertex starts in. Folded to a constant at compile time,
 // so the defaults cost one store of the vertex rather than one per field.
 static constexpr TVertex kPrototype = [] {
  TVertex vertex{};
  if constexpr(requires { vertex.color; }) {
   vertex.color = 0xFFFFFFFFU;
  }
  if constexpr(requires { vertex.normal; }) {
   // Packed (0, 1, 0) — up. A zero normal is NaN after the normalize() every
   // Iris gbuffer vertex stage applies to it. See kDefaultNormal in
   // RenderCore.cpp for the generic-attribute twin.
   vertex.normal = 0x00007F00;
  }
  if constexpr(requires { vertex.midBlock; }) {
   vertex.midBlock = 0;
  }
  if constexpr(requires { vertex.light; }) {
   vertex.light = 0x00F000F0;
  }
  if constexpr(requires { vertex.entity; }) {
   for(auto& value : vertex.entity) value = 0;
  }
  if constexpr(requires { vertex.midU; }) {
   vertex.midU = 0.0f;
   vertex.midV = 0.0f;
  }
  if constexpr(requires { vertex.tangent; }) {
   for(auto& value : vertex.tangent) value = 0;
  }
  return vertex;
 }();
 std::vector<std::uint8_t> buffer_;
 std::size_t vertexCount_ = 0;
 int drawMode_ = 0;
};
} // namespace net::minecraft::client::render
