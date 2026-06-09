#pragma once

#include <cstddef>
#include <cstdint>

namespace net::minecraft::client::gl::GlExtensions {

// Stereo consumers (StereoSession, StereoCompositor, etc.) are out of scope for GL shim work.

// VBO constants
constexpr unsigned int kArrayBuffer = 0x8892;
constexpr unsigned int kStaticDraw = 0x88E4;

// EXT_framebuffer_object constants (OpenGL 1.x extension).
inline constexpr unsigned int GL_FRAMEBUFFER = 0x8D40;
inline constexpr unsigned int GL_RENDERBUFFER = 0x8D41;
inline constexpr unsigned int GL_COLOR_ATTACHMENT0 = 0x8CE0;
inline constexpr unsigned int GL_DEPTH_ATTACHMENT = 0x8D00;
inline constexpr unsigned int GL_FRAMEBUFFER_COMPLETE = 0x8CD5;
inline constexpr unsigned int GL_DEPTH_COMPONENT24 = 0x81A6;

// Loads extension entry points via wglGetProcAddress after a GL context exists.
// Safe to call repeatedly; subsequent calls are no-ops.
void ensureLoaded() noexcept;

[[nodiscard]] bool isVboAvailable() noexcept;
[[nodiscard]] bool isFboAvailable() noexcept;

void genBuffers(int count, unsigned int* names) noexcept;
void deleteBuffers(int count, const unsigned int* names) noexcept;
void bindBuffer(unsigned int target, unsigned int buffer) noexcept;
void bufferData(unsigned int target, std::ptrdiff_t size, const void* data, unsigned int usage) noexcept;

void genFramebuffers(int n, unsigned int* ids) noexcept;
void deleteFramebuffers(int n, const unsigned int* ids) noexcept;
void bindFramebuffer(unsigned int target, unsigned int framebuffer) noexcept;
void framebufferTexture2D(unsigned int target, unsigned int attachment, int textarget, unsigned int texture,
    int level) noexcept;
void genRenderbuffers(int n, unsigned int* ids) noexcept;
void deleteRenderbuffers(int n, const unsigned int* ids) noexcept;
void bindRenderbuffer(unsigned int target, unsigned int renderbuffer) noexcept;
void renderbufferStorage(unsigned int target, unsigned int internalformat, int width, int height) noexcept;
void framebufferRenderbuffer(unsigned int target, unsigned int attachment, unsigned int renderbuffertarget,
    unsigned int renderbuffer) noexcept;
[[nodiscard]] unsigned int checkFramebufferStatus(unsigned int target) noexcept;

} // namespace net::minecraft::client::gl::GlExtensions
