#include "net/minecraft/client/render/ColorSpace.hpp"
#include <string>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"

namespace net::minecraft::client::render {
namespace {

std::string fragmentFor(ColorSpace space) {
 std::string src;
 src += "#define CURRENT_COLOR_SPACE " + std::to_string(static_cast<int>(space)) + "\n";
 src += GlslSnippets::get("colorspace_enum_defines");
 src += GlslSnippets::get("colorspace_fragment");
 return src;
}

bool allocRgba8(gl::GlTexture& tex, int w, int h) {
 if(!tex) {
  const unsigned int h_ = core::genTexture();
  if(h_ == 0) return false;
  tex = gl::GlTexture(h_);
 }
 core::bindTexture(static_cast<int>(tex.handle()));
 ::glTexImage2D(gl::cap::Texture2D, 0, gl::pixel::Rgba8, w, h, 0, gl::pixel::Rgba, gl::pixel::UnsignedByte,
                nullptr);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Nearest);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Nearest);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
 return true;
}

} // namespace

ColorSpaceConverter::~ColorSpaceConverter() {
 destroy();
}

bool ColorSpaceConverter::ready() const noexcept {
 if(!presentFbo_.valid() || !presentTexture_ || !swapTexture_) return false;
 return space_ != ColorSpace::Srgb && program_ && program_->valid();
}

void ColorSpaceConverter::destroy() {
 program_.reset();
 presentTexture_.reset();
 swapTexture_.reset();
 presentFbo_.destroy();
 swapFbo_.destroy();
 width_ = height_ = 0;
 space_ = ColorSpace::Srgb;
}

bool ColorSpaceConverter::ensureProgram(ColorSpace space) {
 if(program_ && space_ == space && program_->valid()) return true;
 auto program = std::make_unique<gl::ShaderProgram>();
 if(!program->compile(GlslSnippets::get("colorspace_vertex"), fragmentFor(space),
                      GlslSnippets::get("colorspace_preamble"))) {
  return false;
 }
 program_ = std::move(program);
 space_ = space;
 return true;
}

bool ColorSpaceConverter::ensureTargets(int width, int height) {
 if(width <= 0 || height <= 0) return false;
 if(presentTexture_ && presentFbo_.valid() && swapTexture_ && swapFbo_.valid() && width_ == width &&
    height_ == height) {
  return true;
 }
 width_ = width;
 height_ = height;
 if(!allocRgba8(presentTexture_, width, height) || !allocRgba8(swapTexture_, width, height)) return false;
 if(!presentFbo_.addColorAttachment(0, presentTexture_.handle()) ||
    !swapFbo_.addColorAttachment(0, swapTexture_.handle())) {
  return false;
 }
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
 return true;
}

void ColorSpaceConverter::rebuild(int width, int height, ColorSpace space) {
 if(!hasGlContext()) return;
 space_ = space;
 if(space == ColorSpace::Srgb) {
  ensureTargets(width, height);
  return;
 }
 if(ensureTargets(width, height)) ensureProgram(space);
}

void ColorSpaceConverter::process(unsigned int targetTexture) {
 if(space_ == ColorSpace::Srgb || targetTexture == 0) return;
 if(!ensureProgram(space_) || !ensureTargets(width_, height_) || !program_ || !swapFbo_.valid()) return;

 const core::DepthScope depthScope(false, false);
 const core::CullScope cullScope(false);
 const core::BlendScope blendScope(false);
 const core::TextureBindScope textureScope;

 swapFbo_.bind();
 core::viewport(0, 0, width_, height_);
 program_->bind();
 core::activeTexture(gl::tex::Texture0);
 core::bindTexture(static_cast<int>(targetTexture));
 program_->set1i("readImage", 0);
 core::drawFullscreen();
 gl::ShaderProgram::unbind();

 swapFbo_.bind();
 core::bindTexture(static_cast<int>(targetTexture));
 ::glCopyTexSubImage2D(gl::cap::Texture2D, 0, 0, 0, 0, 0, width_, height_);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
}

bool ColorSpaceConverter::blitPresentToScreen(int screenWidth, int screenHeight) {
 if(!presentFbo_.valid() || width_ <= 0 || height_ <= 0 || gl::GLCore::blitFramebuffer == nullptr) return false;
 presentFbo_.bindAsReadBuffer();
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::DrawFramebuffer), 0);
 core::viewport(0, 0, screenWidth, screenHeight);
 gl::GLCore::blitFramebuffer(0, 0, width_, height_, 0, 0, screenWidth, screenHeight,
                             gl::attrib::ColorBufferBit, gl::filter::Nearest);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::ReadFramebuffer), 0);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
 return true;
}

} // namespace net::minecraft::client::render
