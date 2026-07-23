#include "net/minecraft/client/render/Framebuffer.hpp"
#include <string>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
namespace net::minecraft::client::render {
namespace {
constexpr unsigned kFramebuffer = 0x8D40;
constexpr unsigned kColorAttachment0 = 0x8CE0;
constexpr unsigned kDepthStencilAttachment = 0x821A;
constexpr unsigned kDepth24Stencil8 = 0x88F0;
constexpr unsigned kDepthStencil = 0x84F9;
constexpr unsigned kUnsignedInt248 = 0x84FA;
constexpr unsigned kFramebufferComplete = 0x8CD5;
constexpr unsigned kTexture2D = 0x0DE1;
constexpr int kRgba8 = 0x8058;
constexpr int kRgba16F = 0x881A;
constexpr unsigned kRgba = 0x1908;
constexpr unsigned kUnsignedByte = 0x1401;
constexpr unsigned kFloat = 0x1406;
constexpr int kFramebufferBinding = 0x8CA6;
constexpr int kViewport = 0x0BA2;
const char* formatName(ColorFormat format) {
 return format == ColorFormat::Rgba16F ? "RGBA16F" : "RGBA8";
}
} // namespace
Framebuffer::~Framebuffer() {
 destroy();
}
bool Framebuffer::allocate(int width, int height, ColorFormat format) {
 if(!gl::GLCore::framebufferSupported) {
  ClientLog::LOGGER.log(LogLevel::Warning,
                        "[shader] framebuffer objects unsupported by driver — offscreen targets unavailable");
  return false;
 }
 const int internalFormat = format == ColorFormat::Rgba16F ? kRgba16F : kRgba8;
 const unsigned pixelType = format == ColorFormat::Rgba16F ? kFloat : kUnsignedByte;
 gl::GLCore::genFramebuffers(1, &fbo_);
 gl::GLCore::bindFramebuffer(kFramebuffer, fbo_);
 colorTexture_ = RenderSystem::genTexture();
 RenderSystem::bindTexture(kTexture2D, static_cast<int>(colorTexture_));
 ::glTexImage2D(kTexture2D, 0, internalFormat, width, height, 0, kRgba, pixelType, nullptr);
 ::glTexParameteri(kTexture2D, 0x2801, 0x2601);
 ::glTexParameteri(kTexture2D, 0x2800, 0x2601);
 ::glTexParameteri(kTexture2D, 0x2802, 0x812F);
 ::glTexParameteri(kTexture2D, 0x2803, 0x812F);
 gl::GLCore::framebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2D, colorTexture_, 0);
 depthStencilTexture_ = RenderSystem::genTexture();
 RenderSystem::bindTexture(kTexture2D, static_cast<int>(depthStencilTexture_));
 ::glTexImage2D(
     kTexture2D, 0, static_cast<int>(kDepth24Stencil8), width, height, 0, kDepthStencil, kUnsignedInt248, nullptr);
 ::glTexParameteri(kTexture2D, 0x2801, 0x2600);
 ::glTexParameteri(kTexture2D, 0x2800, 0x2600);
 ::glTexParameteri(kTexture2D, 0x2802, 0x812F);
 ::glTexParameteri(kTexture2D, 0x2803, 0x812F);
 gl::GLCore::framebufferTexture2D(kFramebuffer, kDepthStencilAttachment, kTexture2D, depthStencilTexture_, 0);
 const unsigned status = gl::GLCore::checkFramebufferStatus(kFramebuffer);
 gl::GLCore::bindFramebuffer(kFramebuffer, 0);
 if(status != kFramebufferComplete) {
  ClientLog::LOGGER.log(LogLevel::Warning,
                        "[shader] framebuffer " + std::to_string(width) + "x" + std::to_string(height) + " " +
                            formatName(format) + " incomplete (status 0x" + std::to_string(status) + ")");
  destroy();
  return false;
 }
 width_ = width;
 height_ = height;
 format_ = format;
 return true;
}
bool Framebuffer::initialize(int width, int height, ColorFormat format) {
 destroy();
 if(width <= 0 || height <= 0) {
  return false;
 }
 valid_ = allocate(width, height, format);
 return valid_;
}
bool Framebuffer::ensure(int width, int height, ColorFormat format) {
 if(valid_ && width == width_ && height == height_ && format == format_) {
  return true;
 }
 return initialize(width, height, format);
}
void Framebuffer::resize(int width, int height, ColorFormat format) {
 if(width <= 0 || height <= 0) {
  return;
 }
 ensure(width, height, format);
}
void Framebuffer::begin() {
 if(!valid_ || active_) {
  return;
 }
 ::glGetIntegerv(static_cast<unsigned>(kFramebufferBinding), &previousBoundFbo_);
 RenderSystem::getIntegerv(static_cast<int>(kViewport), savedViewport_);
 gl::GLCore::bindFramebuffer(kFramebuffer, fbo_);
 RenderSystem::viewport(0, 0, width_, height_);
 active_ = true;
}
void Framebuffer::end() {
 if(!valid_ || !active_) {
  return;
 }
 gl::GLCore::bindFramebuffer(kFramebuffer, static_cast<unsigned>(previousBoundFbo_));
 RenderSystem::viewport(savedViewport_[0], savedViewport_[1], savedViewport_[2], savedViewport_[3]);
 active_ = false;
}
void Framebuffer::bindTarget() {
 if(!valid_) {
  return;
 }
 gl::GLCore::bindFramebuffer(kFramebuffer, fbo_);
 RenderSystem::viewport(0, 0, width_, height_);
}
void Framebuffer::blitToScreen(int screenWidth, int screenHeight) {
 if(!valid_ || colorTexture_ == 0) {
  return;
 }
 const RenderSystem::StateShadow saved = RenderSystem::getShadow();
 RenderSystem::disableDepthTest();
 RenderSystem::disableCull();
 RenderSystem::disableBlend();
 RenderSystem::depthMask(false);
 RenderSystem::viewport(0, 0, screenWidth, screenHeight);
 RenderSystem::activeTexture(0x84C0);
 RenderSystem::bindTexture(kTexture2D, static_cast<int>(colorTexture_));
 gl::engine_pipeline::blitFullscreen();
 RenderSystem::setShadow(saved);
}
void Framebuffer::destroy() {
 if(active_) {
  end();
 }
 if(fbo_ != 0) {
  gl::GLCore::deleteFramebuffers(1, &fbo_);
  fbo_ = 0;
 }
 if(depthStencilTexture_ != 0) {
  RenderSystem::deleteTexture(depthStencilTexture_);
  depthStencilTexture_ = 0;
 }
 if(colorTexture_ != 0) {
  RenderSystem::deleteTexture(colorTexture_);
  colorTexture_ = 0;
 }
 width_ = 0;
 height_ = 0;
 valid_ = false;
}
} // namespace net::minecraft::client::render
