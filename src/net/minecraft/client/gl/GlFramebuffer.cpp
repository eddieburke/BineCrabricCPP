#include "net/minecraft/client/gl/GlFramebuffer.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
namespace net::minecraft::client::gl {
namespace {
unsigned boundFramebuffer() {
  int bound = 0;
  ::glGetIntegerv(static_cast<unsigned>(query::FramebufferBinding), &bound);
  return static_cast<unsigned>(bound);
}
} // namespace
GlFramebuffer::~GlFramebuffer() {
  destroy();
}
void GlFramebuffer::destroy() {
  if(id_ != 0) {
    GLCore::deleteFramebuffers(1, &id_);
    id_ = 0;
  }
  attachments_.clear();
  hasDepthAttachment_ = false;
}
bool GlFramebuffer::ensureCreated() {
  if(id_ != 0) {
    return true;
  }
  if(GLCore::genFramebuffers == nullptr || GLCore::bindFramebuffer == nullptr) {
    return false;
  }
  GLCore::genFramebuffers(1, &id_);
  // Java queries GL_MAX_DRAW_BUFFERS/GL_MAX_COLOR_ATTACHMENTS once at construction
  // (GlFramebuffer.java:24-25); GL_MAX_DRAW_BUFFERS is at least 8.
  int maxDraw = 8;
  int maxColor = 8;
  ::glGetIntegerv(static_cast<unsigned>(framebuffer::MaxDrawBuffers), &maxDraw);
  ::glGetIntegerv(static_cast<unsigned>(framebuffer::MaxColorAttachments), &maxColor);
  maxDrawBuffers_ = maxDraw > 0 ? maxDraw : 8;
  maxColorAttachments_ = maxColor > 0 ? maxColor : 8;
  return id_ != 0;
}
bool GlFramebuffer::addDepthAttachment(unsigned texture) {
  if(!ensureCreated()) {
    return false;
  }
  // Java addDepthAttachment (GlFramebuffer.java:29-40): GL_DEPTH_ATTACHMENT with a
  // TODO for combined depth-stencil formats; texture 0 detaches (C++ extension for the
  // shared write FBO, which drops the gbuffer depth between passes).
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::Framebuffer), id_);
  GLCore::framebufferTexture2D(static_cast<unsigned>(framebuffer::Framebuffer),
                               static_cast<unsigned>(framebuffer::DepthAttachment),
                               static_cast<unsigned>(cap::Texture2D), texture, 0);
  hasDepthAttachment_ = texture != 0;
  return true;
}
bool GlFramebuffer::removeDepthAttachment() {
  if(!ensureCreated()) {
    return false;
  }
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::Framebuffer), id_);
  GLCore::framebufferTexture2D(static_cast<unsigned>(framebuffer::Framebuffer),
                               static_cast<unsigned>(framebuffer::DepthAttachment),
                               static_cast<unsigned>(cap::Texture2D), 0, 0);
  hasDepthAttachment_ = false;
  return true;
}
bool GlFramebuffer::addColorAttachment(int index, unsigned texture) {
  if(!ensureCreated()) {
    return false;
  }
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::Framebuffer), id_);
  GLCore::framebufferTexture2D(static_cast<unsigned>(framebuffer::Framebuffer),
                               static_cast<unsigned>(framebuffer::ColorAttachment0) + static_cast<unsigned>(index),
                               static_cast<unsigned>(cap::Texture2D), texture, 0);
  attachments_[index] = texture;
  return true;
}
bool GlFramebuffer::noDrawBuffers() {
  // Java noDrawBuffers passes a single GL_NONE straight to drawBuffers (not through
  // the index-mapping overload): GlFramebuffer.java:57-59.
  if(!ensureCreated() || GLCore::drawBuffers == nullptr) {
    return false;
  }
  constexpr unsigned none = 0; // GL_NONE
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::Framebuffer), id_);
  GLCore::drawBuffers(1, &none);
  return true;
}
bool GlFramebuffer::drawBuffers(const std::vector<int>& buffers) {
  if(!ensureCreated()) {
    return false;
  }
  // Java throws IllegalArgumentException on both limit violations; the C++ callers log.
  if(buffers.size() > static_cast<std::size_t>(maxDrawBuffers_)) {
    return false;
  }
  for(int buffer : buffers) {
    if(buffer >= maxColorAttachments_) {
      return false;
    }
  }
  if(GLCore::drawBuffers == nullptr) {
    return false;
  }
  std::vector<unsigned> glBuffers;
  glBuffers.reserve(buffers.size());
  for(int buffer : buffers) {
    glBuffers.push_back(static_cast<unsigned>(framebuffer::ColorAttachment0) + static_cast<unsigned>(buffer));
  }
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::Framebuffer), id_);
  GLCore::drawBuffers(static_cast<int>(glBuffers.size()), glBuffers.data());
  return true;
}
void GlFramebuffer::readBuffer(int buffer) {
  if(!ensureCreated()) {
    return;
  }
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::Framebuffer), id_);
  ::glReadBuffer(static_cast<unsigned>(framebuffer::ColorAttachment0) + static_cast<unsigned>(buffer));
}
void GlFramebuffer::bind() const {
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::Framebuffer), id_);
}
void GlFramebuffer::bindAsReadBuffer() const {
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::ReadFramebuffer), id_);
}
void GlFramebuffer::bindAsDrawBuffer() const {
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::DrawFramebuffer), id_);
}
unsigned GlFramebuffer::checkStatus() const {
  // Java getStatus: bind() then checkFramebufferStatus (GlFramebuffer.java:108-112).
  GLCore::bindFramebuffer(static_cast<unsigned>(framebuffer::Framebuffer), id_);
  return GLCore::checkFramebufferStatus(static_cast<unsigned>(framebuffer::Framebuffer));
}
unsigned GlFramebuffer::colorAttachment(int index) const noexcept {
  const auto found = attachments_.find(index);
  return found == attachments_.end() ? 0 : found->second;
}
} // namespace net::minecraft::client::gl
