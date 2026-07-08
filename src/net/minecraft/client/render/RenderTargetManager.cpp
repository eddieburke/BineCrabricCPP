#include "net/minecraft/client/render/RenderTargetManager.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include <algorithm>
namespace net::minecraft::client::render {
bool RenderTarget::initialize(int widthIn, int heightIn) {
  destroy();
  gl::GLCore::ensureLoaded();
  if(!gl::GLCore::framebufferSupported || widthIn <= 0 || heightIn <= 0) {
    return false;
  }
  width = widthIn;
  height = heightIn;
  gl::genFramebuffers(1, &fbo);
  gl::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
  gl::genTextures(1, &colorTexture);
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(colorTexture));
  gl::texImage2D(gl::cap::Texture2D, 0, gl::pixel::Rgb, width, height, 0, gl::pixel::Rgb, gl::pixel::UnsignedByte,
                 nullptr);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Linear);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Linear);
  gl::framebufferTexture2D(gl::framebuffer::Framebuffer, gl::framebuffer::ColorAttachment0, gl::cap::Texture2D,
                           colorTexture, 0);
  gl::genRenderbuffers(1, &depthStencilRb);
  gl::bindRenderbuffer(gl::framebuffer::Renderbuffer, depthStencilRb);
  gl::renderbufferStorage(gl::framebuffer::Renderbuffer, gl::framebuffer::Depth24Stencil8, width, height);
  gl::framebufferRenderbuffer(gl::framebuffer::Framebuffer, gl::framebuffer::DepthStencilAttachment,
                              gl::framebuffer::Renderbuffer, depthStencilRb);
  const bool ok = gl::checkFramebufferStatus(gl::framebuffer::Framebuffer) == gl::framebuffer::Complete;
  gl::bindRenderbuffer(gl::framebuffer::Renderbuffer, 0);
  gl::bindFramebuffer(gl::framebuffer::Framebuffer, 0);
  if(!ok) {
    destroy();
  }
  return ok;
}
void RenderTarget::destroy() {
  if(depthStencilRb != 0) {
    gl::deleteRenderbuffers(1, &depthStencilRb);
    depthStencilRb = 0;
  }
  if(colorTexture != 0) {
    gl::deleteTextures(1, &colorTexture);
    colorTexture = 0;
  }
  if(fbo != 0) {
    gl::deleteFramebuffers(1, &fbo);
    fbo = 0;
  }
  width = 0;
  height = 0;
}
void RenderTarget::bind() const {
  gl::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
}
void RenderTarget::unbind() {
  gl::bindFramebuffer(gl::framebuffer::Framebuffer, 0);
}
int RenderTargetManager::create(int width, int height) {
  RenderTarget target;
  if(!target.initialize(width, height)) {
    return -1;
  }
  const int handle = nextHandle_++;
  targets_.emplace(handle, std::move(target));
  return handle;
}
bool RenderTargetManager::destroy(int handle) {
  auto it = targets_.find(handle);
  if(it == targets_.end()) {
    return false;
  }
  it->second.destroy();
  targets_.erase(it);
  return true;
}
int RenderTargetManager::textureId(int handle) const {
  const auto it = targets_.find(handle);
  if(it == targets_.end()) {
    return -1;
  }
  return static_cast<int>(it->second.colorTexture);
}
bool RenderTargetManager::render(int handle, GameRenderer& renderer, float tickDelta, double x, double y, double z,
                                 float yaw, float pitch, float roll, float fov) {
  auto it = targets_.find(handle);
  if(it == targets_.end() || it->second.fbo == 0) {
    return false;
  }
  FrameRenderCamera camera;
  camera.x = x;
  camera.y = y;
  camera.z = z;
  camera.yaw = yaw;
  camera.pitch = pitch;
  camera.roll = roll;
  camera.customView = true;
  camera.hideFirstPersonHand = true;
  const float clampedFov = std::clamp(fov, 1.0f, 179.0f);
  it->second.bind();
  renderingHandle_ = handle;
  renderer.renderToCurrentTarget(tickDelta, camera, clampedFov, it->second.width, it->second.height, true);
  renderingHandle_ = -1;
  RenderTarget::unbind();
  return true;
}
} // namespace net::minecraft::client::render
