#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include <cstdint>
#include <vector>
#include "net/minecraft/client/gl/GLCore.hpp"
namespace net::minecraft::client::render::quad_index {
namespace {
constexpr unsigned kElementArrayBuffer = 0x8893;
constexpr unsigned kStaticDraw = 0x88E4;
unsigned sHandle = 0;
std::size_t sVertexCapacity = 0;
} // namespace
bool ensure(std::size_t vertexCount) {
 if(gl::GLCore::genBuffers == nullptr || gl::GLCore::bufferData == nullptr) {
  return false;
 }
 if(sHandle != 0 && vertexCount <= sVertexCapacity) {
  return true;
 }
 std::size_t newCapacity = sVertexCapacity < 16384 ? 16384 : sVertexCapacity;
 while(newCapacity < vertexCount) {
  newCapacity *= 2;
 }
 const std::size_t quadCount = newCapacity / 4;
 std::vector<std::uint32_t> indices(quadCount * 6);
 for(std::size_t quad = 0; quad < quadCount; ++quad) {
  const std::uint32_t base = static_cast<std::uint32_t>(quad * 4);
  std::uint32_t* out = indices.data() + quad * 6;
  out[0] = base;
  out[1] = base + 1;
  out[2] = base + 2;
  out[3] = base;
  out[4] = base + 2;
  out[5] = base + 3;
 }
 if(sHandle == 0) {
  gl::GLCore::genBuffers(1, &sHandle);
 }
 gl::GLCore::bindBuffer(kElementArrayBuffer, sHandle);
 gl::GLCore::bufferData(kElementArrayBuffer,
                        static_cast<intptr_t>(indices.size() * sizeof(std::uint32_t)),
                        indices.data(),
                        kStaticDraw);
 sVertexCapacity = newCapacity;
 return true;
}
unsigned handle() noexcept {
 return sHandle;
}
} // namespace net::minecraft::client::render::quad_index
