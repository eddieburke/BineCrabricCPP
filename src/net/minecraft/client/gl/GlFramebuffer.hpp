#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
namespace net::minecraft::client::gl {
// Mirrors Iris Java gl/framebuffer/GlFramebuffer.java: an owned GL framebuffer with
// attachment tracking, draw-buffer validation against the GPU limits and bind helpers.
// Java allocates the GL id in the constructor and throws IllegalArgumentException on
// limit violations; the C++ port allocates lazily (so destroy()/re-ensure cycles work)
// and reports failures through return values that the callers log.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/framebuffer/GlFramebuffer.java
class GlFramebuffer {
 public:
  GlFramebuffer() = default;
  ~GlFramebuffer();
  GlFramebuffer(const GlFramebuffer&) = delete;
  GlFramebuffer& operator=(const GlFramebuffer&) = delete;
  // Deletes the GL object and resets all attachment state (Java destroy()).
  void destroy();
  // Java addDepthAttachment; texture 0 detaches (Java only ever attaches real textures;
  // the shared write FBO needs this to drop the gbuffer depth between passes).
  bool addDepthAttachment(unsigned texture);
  bool removeDepthAttachment();
  bool addColorAttachment(int index, unsigned texture);
  // Java noDrawBuffers: draws to GL_NONE only.
  bool noDrawBuffers();
  // Java drawBuffers with the limit checks (returns false instead of throwing when a
  // buffer index is at/above GL_MAX_COLOR_ATTACHMENTS or the count exceeds
  // GL_MAX_DRAW_BUFFERS).
  bool drawBuffers(const std::vector<int>& buffers);
  void readBuffer(int buffer);
  void bind() const;
  void bindAsReadBuffer() const;
  void bindAsDrawBuffer() const;
  // Java getStatus: binds the framebuffer (leaving it bound) and returns
  // glCheckFramebufferStatus(GL_FRAMEBUFFER).
  [[nodiscard]] unsigned checkStatus() const;
  [[nodiscard]] unsigned id() const noexcept { return id_; }
  [[nodiscard]] bool hasDepthAttachment() const noexcept { return hasDepthAttachment_; }
  [[nodiscard]] unsigned colorAttachment(int index) const noexcept;
  [[nodiscard]] bool valid() const noexcept { return id_ != 0; }

 private:
  bool ensureCreated();
  unsigned id_ = 0;
  std::unordered_map<int, unsigned> attachments_;
  bool hasDepthAttachment_ = false;
  int maxDrawBuffers_ = 8;
  int maxColorAttachments_ = 8;
};
} // namespace net::minecraft::client::gl
