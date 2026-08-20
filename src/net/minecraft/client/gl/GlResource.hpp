#pragma once
#include <cstdint>
#include <utility>
#include "net/minecraft/client/gl/GLCore.hpp"
namespace net::minecraft::client::render::core {
unsigned int genTexture();
void deleteTexture(unsigned int texture);
} // namespace net::minecraft::client::render::core
namespace net::minecraft::client::gl {
// Move-only RAII handle for a GL object. Ownership transfers via std::move are
// safe and leaks on partial-construction/error paths disappear. Only the delete
// call differs per object kind, so that is the whole of the policy — the four
// handle types below were previously four byte-identical classes.
//
// Every deleter null-checks its GLCore function pointer because handles may be
// destroyed after GL teardown, or after the GLCore statics themselves
// (process-global wrappers).
template <class Deleter>
class GlHandle {
 public:
 GlHandle() = default;
 explicit GlHandle(unsigned int handle) noexcept : handle_(handle) {}
 ~GlHandle() {
  reset();
 }
 GlHandle(const GlHandle&) = delete;
 GlHandle& operator=(const GlHandle&) = delete;
 GlHandle(GlHandle&& other) noexcept : handle_(other.release()) {}
 GlHandle& operator=(GlHandle&& other) noexcept {
  if(this != &other) {
   reset();
   handle_ = other.release();
  }
  return *this;
 }
 void reset() noexcept {
  if(handle_ != 0) {
   Deleter{}(handle_);
   handle_ = 0;
  }
 }
 [[nodiscard]] unsigned int release() noexcept {
  const unsigned int handle = handle_;
  handle_ = 0;
  return handle;
 }
 [[nodiscard]] unsigned int handle() const noexcept {
  return handle_;
 }
 [[nodiscard]] explicit operator bool() const noexcept {
  return handle_ != 0;
 }

 private:
 unsigned int handle_ = 0;
};
// Textures deliberately route through render::core::deleteTexture instead of a raw
// glDeleteTextures: core keeps the bind-cache and allocation registry coherent
// (unbind-before-delete + g_textureUnitOf purge) and sweeps leftovers at shutdown.
struct GlTextureDeleter {
 void operator()(unsigned int handle) const noexcept {
  render::core::deleteTexture(handle);
 }
};
struct GlBufferDeleter {
 void operator()(unsigned int handle) const noexcept {
  if(GLCore::deleteBuffers != nullptr) {
   GLCore::deleteBuffers(1, &handle);
  }
 }
};
struct GlVaoDeleter {
 void operator()(unsigned int handle) const noexcept {
  if(GLCore::deleteVertexArrays != nullptr) {
   GLCore::deleteVertexArrays(1, &handle);
  }
 }
};
struct GlSamplerDeleter {
 void operator()(unsigned int handle) const noexcept {
  if(GLCore::deleteSamplers != nullptr) {
   GLCore::deleteSamplers(1, &handle);
  }
 }
};
using GlTexture = GlHandle<GlTextureDeleter>;
using GlBuffer = GlHandle<GlBufferDeleter>;
using GlVao = GlHandle<GlVaoDeleter>;
using GlSampler = GlHandle<GlSamplerDeleter>;
} // namespace net::minecraft::client::gl
