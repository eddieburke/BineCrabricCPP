#pragma once
#include <cstdint>
#include <utility>
#include "net/minecraft/client/gl/GLCore.hpp"
namespace net::minecraft::client::render::core {
unsigned int genTexture();
void deleteTexture(unsigned int texture);
} // namespace net::minecraft::client::render::core
namespace net::minecraft::client::gl {
// Move-only RAII handles for GL objects. Each wrapper deletes its backing object
// on destruction (and on reset()), so ownership transfers via std::move are safe
// and leaks on partial-construction/error paths disappear. All destructors
// null-check the GLCore function pointer because these may be destroyed after GL
// teardown or after the GLCore statics themselves (process-global wrappers).
//
// Textures deliberately route through render::core::genTexture/deleteTexture
// instead of a raw glDeleteTextures: core keeps the bind-cache and allocation
// registry coherent (unbind-before-delete + g_textureUnitOf purge) and sweeps
// leftovers at shutdown.
class GlTexture {
 public:
 GlTexture() = default;
 explicit GlTexture(unsigned int handle) noexcept : handle_(handle) {}
 ~GlTexture() {
  reset();
 }
 GlTexture(const GlTexture&) = delete;
 GlTexture& operator=(const GlTexture&) = delete;
 GlTexture(GlTexture&& other) noexcept : handle_(other.release()) {}
 GlTexture& operator=(GlTexture&& other) noexcept {
  if(this != &other) {
   reset();
   handle_ = other.release();
  }
  return *this;
 }
 void reset() noexcept {
  if(handle_ != 0) {
   render::core::deleteTexture(handle_);
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
 [[nodiscard]] unsigned int* handlePtr() noexcept {
  return &handle_;
 }
 [[nodiscard]] explicit operator bool() const noexcept {
  return handle_ != 0;
 }

 private:
 unsigned int handle_ = 0;
};
class GlBuffer {
 public:
 GlBuffer() = default;
 explicit GlBuffer(unsigned int handle) noexcept : handle_(handle) {}
 ~GlBuffer() {
  reset();
 }
 GlBuffer(const GlBuffer&) = delete;
 GlBuffer& operator=(const GlBuffer&) = delete;
 GlBuffer(GlBuffer&& other) noexcept : handle_(other.release()) {}
 GlBuffer& operator=(GlBuffer&& other) noexcept {
  if(this != &other) {
   reset();
   handle_ = other.release();
  }
  return *this;
 }
 void reset() noexcept {
  if(handle_ != 0) {
   if(GLCore::deleteBuffers != nullptr) {
    GLCore::deleteBuffers(1, &handle_);
   }
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
class GlVao {
 public:
 GlVao() = default;
 explicit GlVao(unsigned int handle) noexcept : handle_(handle) {}
 ~GlVao() {
  reset();
 }
 GlVao(const GlVao&) = delete;
 GlVao& operator=(const GlVao&) = delete;
 GlVao(GlVao&& other) noexcept : handle_(other.release()) {}
 GlVao& operator=(GlVao&& other) noexcept {
  if(this != &other) {
   reset();
   handle_ = other.release();
  }
  return *this;
 }
 void reset() noexcept {
  if(handle_ != 0) {
   if(GLCore::deleteVertexArrays != nullptr) {
    GLCore::deleteVertexArrays(1, &handle_);
   }
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
class GlSampler {
 public:
 GlSampler() = default;
 explicit GlSampler(unsigned int handle) noexcept : handle_(handle) {}
 ~GlSampler() {
  reset();
 }
 GlSampler(const GlSampler&) = delete;
 GlSampler& operator=(const GlSampler&) = delete;
 GlSampler(GlSampler&& other) noexcept : handle_(other.release()) {}
 GlSampler& operator=(GlSampler&& other) noexcept {
  if(this != &other) {
   reset();
   handle_ = other.release();
  }
  return *this;
 }
 void reset() noexcept {
  if(handle_ != 0) {
   if(GLCore::deleteSamplers != nullptr) {
    GLCore::deleteSamplers(1, &handle_);
   }
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
} // namespace net::minecraft::client::gl
