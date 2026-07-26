#include "net/minecraft/client/render/RenderTargets.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
namespace net::minecraft::client::render {
namespace {
constexpr int kFramebufferBinding = 0x8CA6;
constexpr int kRenderbufferBinding = 0x8CA7;
constexpr int kViewport = 0x0BA2;
bool finite(float value) {
 return std::isfinite(value);
}
bool finite(double value) {
 return std::isfinite(value);
}
} // namespace
bool RenderTarget::initialize(int widthIn, int heightIn, int colorCount, bool useDepthTexture) {
 destroy();
 gl::GLCore::ensureLoaded();
 if(!gl::GLCore::framebufferSupported || widthIn <= 0 || heightIn <= 0 || colorCount < 0 ||
    (colorCount == 0 && !useDepthTexture)) {
  return false;
 }
 int previousFbo = 0;
 int previousRenderbuffer = 0;
 ::glGetIntegerv(static_cast<unsigned>(kFramebufferBinding), &previousFbo);
 ::glGetIntegerv(static_cast<unsigned>(kRenderbufferBinding), &previousRenderbuffer);
 const RenderSystem::StateShadow previousState = RenderSystem::getShadow();
 width = widthIn;
 height = heightIn;
 gl::GLCore::genFramebuffers(1, &fbo);
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
 colorTextures.resize(static_cast<std::size_t>(colorCount), 0);
 std::vector<unsigned int> drawBuffers;
 drawBuffers.reserve(static_cast<std::size_t>(colorCount));
 for(int i = 0; i < colorCount; ++i) {
  colorTextures[static_cast<std::size_t>(i)] = RenderSystem::genTexture();
  RenderSystem::bindTexture(gl::cap::Texture2D, static_cast<int>(colorTextures[static_cast<std::size_t>(i)]));
  ::glTexImage2D(gl::cap::Texture2D,
                 0,
                 gl::pixel::Rgba,
                 width,
                 height,
                 0,
                 gl::pixel::Rgba,
                 gl::pixel::UnsignedByte,
                 nullptr);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Linear);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Linear);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
  gl::GLCore::framebufferTexture2D(gl::framebuffer::Framebuffer,
                                   static_cast<unsigned>(gl::framebuffer::ColorAttachment0 + i),
                                   gl::cap::Texture2D,
                                   colorTextures[static_cast<std::size_t>(i)],
                                   0);
  drawBuffers.push_back(static_cast<unsigned>(gl::framebuffer::ColorAttachment0 + i));
 }
  if(colorCount > 0) {
    if(gl::GLCore::drawBuffers != nullptr) {
      gl::GLCore::drawBuffers(colorCount, drawBuffers.data());
    }
  } else {
    ::glDrawBuffer(0);
    ::glReadBuffer(0);
  }
 if(useDepthTexture) {
  depthTexture = RenderSystem::genTexture();
  RenderSystem::bindTexture(gl::cap::Texture2D, static_cast<int>(depthTexture));
  ::glTexImage2D(gl::cap::Texture2D,
                 0,
                 gl::pixel::DepthComponent24,
                 width,
                 height,
                 0,
                 gl::pixel::DepthComponent,
                 gl::pixel::Float,
                 nullptr);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Nearest);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Nearest);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
  gl::GLCore::framebufferTexture2D(
      gl::framebuffer::Framebuffer, gl::framebuffer::DepthAttachment, gl::cap::Texture2D, depthTexture, 0);
 } else {
  gl::GLCore::genRenderbuffers(1, &depthStencilRb);
  gl::GLCore::bindRenderbuffer(gl::framebuffer::Renderbuffer, depthStencilRb);
  gl::GLCore::renderbufferStorage(gl::framebuffer::Renderbuffer, gl::framebuffer::Depth24Stencil8, width, height);
  gl::GLCore::framebufferRenderbuffer(gl::framebuffer::Framebuffer,
                                      gl::framebuffer::DepthStencilAttachment,
                                      gl::framebuffer::Renderbuffer,
                                      depthStencilRb);
 }
 const bool ok = gl::GLCore::checkFramebufferStatus(gl::framebuffer::Framebuffer) ==
                 static_cast<unsigned>(gl::framebuffer::Complete);
 gl::GLCore::bindRenderbuffer(gl::framebuffer::Renderbuffer, static_cast<unsigned>(previousRenderbuffer));
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, static_cast<unsigned>(previousFbo));
 RenderSystem::setShadow(previousState);
 if(!ok) {
  destroy();
 }
 return ok;
}
bool RenderTarget::resize(int newWidth, int newHeight) {
 if(width == newWidth && height == newHeight) {
  return true;
 }
 const int colorCount = static_cast<int>(colorTextures.size());
 const bool useDepth = depthTexture != 0;
 destroy();
 return initialize(newWidth, newHeight, colorCount, useDepth);
}
void RenderTarget::destroy() {
 if(depthStencilRb != 0) {
  gl::GLCore::deleteRenderbuffers(1, &depthStencilRb);
  depthStencilRb = 0;
 }
 if(depthTexture != 0) {
  RenderSystem::deleteTexture(depthTexture);
  depthTexture = 0;
 }
 for(unsigned int texture : colorTextures) {
  if(texture != 0) {
   RenderSystem::deleteTexture(texture);
  }
 }
 colorTextures.clear();
 if(fbo != 0) {
  gl::GLCore::deleteFramebuffers(1, &fbo);
  fbo = 0;
 }
 width = 0;
 height = 0;
}
void RenderTarget::bind() const {
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
}
void RenderTarget::unbind() {
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, 0);
}
int FramebufferManager::create(int width, int height, int colorCount, bool useDepthTexture) {
 RenderTarget target;
 if(!target.initialize(width, height, colorCount, useDepthTexture)) {
  return -1;
 }
 const int handle = nextHandle_++;
 targets_.emplace(handle, std::move(target));
 return handle;
}
bool FramebufferManager::destroy(int handle) {
 auto it = targets_.find(handle);
 if(it == targets_.end()) {
  return false;
 }
 it->second.destroy();
 targets_.erase(it);
 return true;
}
int FramebufferManager::textureId(int handle, int attachmentIndex) const {
 const auto it = targets_.find(handle);
 if(it == targets_.end() || attachmentIndex < 0 ||
    attachmentIndex >= static_cast<int>(it->second.colorTextures.size())) {
  return -1;
 }
 return static_cast<int>(it->second.colorTextures[static_cast<std::size_t>(attachmentIndex)]);
}
int FramebufferManager::depthTextureId(int handle) const {
 const auto it = targets_.find(handle);
 if(it == targets_.end() || it->second.depthTexture == 0) {
  return -1;
 }
 return static_cast<int>(it->second.depthTexture);
}
bool FramebufferManager::resize(int handle, int newWidth, int newHeight) {
 auto it = targets_.find(handle);
 if(it == targets_.end()) {
  return false;
 }
 return it->second.resize(newWidth, newHeight);
}
bool FramebufferManager::bind(int handle) const {
 const auto it = targets_.find(handle);
 if(it == targets_.end() || it->second.fbo == 0) {
  return false;
 }
 it->second.bind();
 return true;
}
void FramebufferManager::unbind() {
 RenderTarget::unbind();
}
int FramebufferManager::width(int handle) const {
 const auto it = targets_.find(handle);
 return it != targets_.end() ? it->second.width : 0;
}
int FramebufferManager::height(int handle) const {
 const auto it = targets_.find(handle);
 return it != targets_.end() ? it->second.height : 0;
}
bool FramebufferManager::renderWorldTo(int handle,
                                       GameRenderer& renderer,
                                       float tickDelta,
                                       double x,
                                       double y,
                                       double z,
                                       float yaw,
                                       float pitch,
                                       float roll,
                                       float fov,
                                       bool orthographic,
                                       float orthoHalfWidth,
                                       float orthoHalfHeight,
                                       float orthoNear,
                                       float orthoFar,
                                       bool shadowPass,
                                       bool shadowEntities,
                                       float perspectiveNear,
                                       float perspectiveFar,
                                       int excludedEntityId,
                                       FrameRenderCamera* renderedCamera) {
 auto it = targets_.find(handle);
 if(it == targets_.end() || it->second.fbo == 0 || it->second.width <= 0 || it->second.height <= 0 ||
    renderer.client == nullptr || renderingHandle_ != -1 || !finite(tickDelta) || !finite(x) || !finite(y) ||
    !finite(z) || !finite(yaw) || !finite(pitch) || !finite(roll) || !finite(fov) ||
    (orthographic && (!finite(orthoHalfWidth) || !finite(orthoHalfHeight) || !finite(orthoNear) ||
                      !finite(orthoFar) || orthoHalfWidth <= 0.0f || orthoHalfHeight <= 0.0f ||
                      orthoNear == orthoFar)) ||
    (!orthographic && (!finite(perspectiveNear) || !finite(perspectiveFar)))) {
  return false;
 }
 int prevFbo = 0;
 ::glGetIntegerv(static_cast<unsigned>(kFramebufferBinding), &prevFbo);
 int prevViewport[4] = {0, 0, 0, 0};
 RenderSystem::getIntegerv(kViewport, prevViewport);
 const RenderSystem::StateShadow prevRenderState = RenderSystem::getShadow();
 const net::minecraft::util::math::MatrixStack prevModelView = net::minecraft::util::math::g_modelView;
 const net::minecraft::util::math::MatrixStack prevProjection = net::minecraft::util::math::g_projection;
 const int prevRenderingHandle = renderingHandle_;
 const FrameRenderCamera prevFrameCamera = renderer.frameCamera_;
 const FrameRenderCamera prevPublishedCamera = RenderCameraState::instance().frame();
 WorldRenderer* worldRenderer = renderer.client->worldRenderer.get();
 net::minecraft::Entity* prevWorldCam = nullptr;
 bool prevRenderCamEntity = false;
 int prevExcludedEntityId = -1;
 double prevFrameCamX = 0.0;
 double prevFrameCamY = 0.0;
 double prevFrameCamZ = 0.0;
 bool prevHasFrameCam = false;
 if(worldRenderer != nullptr) {
  prevWorldCam = worldRenderer->cameraEntity_;
  prevRenderCamEntity = worldRenderer->renderCameraEntity_;
  prevExcludedEntityId = worldRenderer->excludedEntityId_;
  prevFrameCamX = worldRenderer->frameCamX_;
  prevFrameCamY = worldRenderer->frameCamY_;
  prevFrameCamZ = worldRenderer->frameCamZ_;
  prevHasFrameCam = worldRenderer->hasFrameCamera_;
  worldRenderer->pushCullState();
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
 camera.orthographic = orthographic;
 camera.orthoHalfWidth = std::max(0.01f, orthoHalfWidth);
 camera.orthoHalfHeight = std::max(0.01f, orthoHalfHeight);
 camera.orthoNear = orthoNear;
 camera.orthoFar = orthoFar;
 camera.shadowPass = shadowPass;
 camera.shadowEntities = shadowEntities;
 if(perspectiveNear > 0.0f) {
  camera.perspectiveNear = perspectiveNear;
 }
 if(perspectiveFar > 0.0f) {
  camera.perspectiveFar = perspectiveFar;
 }
 if(worldRenderer != nullptr) {
  worldRenderer->excludedEntityId_ = excludedEntityId;
 }
 const float clampedFov = std::clamp(fov, 1.0f, 179.0f);
 it->second.bind();
 RenderSystem::viewport(0, 0, it->second.width, it->second.height);
 renderingHandle_ = handle;
 renderer.renderToCurrentTarget(std::clamp(tickDelta, 0.0f, 1.0f),
                                camera,
                                clampedFov,
                                it->second.width,
                                it->second.height,
                                true);
 if(renderedCamera != nullptr) {
  *renderedCamera = renderer.frameCamera_;
 }
 renderingHandle_ = prevRenderingHandle;
 if(worldRenderer != nullptr) {
  worldRenderer->cameraEntity_ = prevWorldCam;
  worldRenderer->renderCameraEntity_ = prevRenderCamEntity;
  worldRenderer->excludedEntityId_ = prevExcludedEntityId;
  worldRenderer->frameCamX_ = prevFrameCamX;
  worldRenderer->frameCamY_ = prevFrameCamY;
  worldRenderer->frameCamZ_ = prevFrameCamZ;
  worldRenderer->hasFrameCamera_ = prevHasFrameCam;
  worldRenderer->popCullState();
 }
 renderer.frameCamera_ = prevFrameCamera;
 RenderCameraState::instance().setFrame(prevPublishedCamera);
 net::minecraft::util::math::g_modelView = prevModelView;
 net::minecraft::util::math::g_projection = prevProjection;
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, static_cast<unsigned>(prevFbo));
 RenderSystem::setShadow(prevRenderState);
 RenderSystem::matrixMode(gl::matrix_::ModelView);
 RenderSystem::viewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
 return true;
}
} // namespace net::minecraft::client::render
