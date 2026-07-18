#include "net/minecraft/client/render/Framebuffer.hpp"
#include <algorithm>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/LegacyChunkCache.hpp"
namespace net::minecraft::client::render {
bool Framebuffer::initialize(int widthIn, int heightIn, int colorCount, bool useDepthTexture) {
  destroy();
  gl::GLCore::ensureLoaded();
  if(!gl::GLCore::framebufferSupported || widthIn <= 0 || heightIn <= 0 || colorCount <= 0) {
    return false;
  }
  width = widthIn;
  height = heightIn;
  gl::genFramebuffers(1, &fbo);
  gl::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
  colorTextures.resize(colorCount, 0);
  std::vector<unsigned int> drawBuffers;
  drawBuffers.reserve(colorCount);
  for(int i = 0; i < colorCount; ++i) {
    gl::genTextures(1, &colorTextures[i]);
    gl::bindTexture(gl::cap::Texture2D, static_cast<int>(colorTextures[i]));
    gl::texImage2D(gl::cap::Texture2D, 0, gl::pixel::Rgba, width, height, 0, gl::pixel::Rgba, gl::pixel::UnsignedByte, nullptr);
    gl::texParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Linear);
    gl::texParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Linear);
    gl::texParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
    gl::texParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
    gl::framebufferTexture2D(gl::framebuffer::Framebuffer, gl::framebuffer::ColorAttachment0 + i, gl::cap::Texture2D, colorTextures[i], 0);
    drawBuffers.push_back(gl::framebuffer::ColorAttachment0 + i);
  }
  if(gl::GLCore::drawBuffers && colorCount > 0) {
    gl::GLCore::drawBuffers(colorCount, drawBuffers.data());
  }
  if(useDepthTexture) {
    gl::genTextures(1, &depthTexture);
    gl::bindTexture(gl::cap::Texture2D, static_cast<int>(depthTexture));
    // 0x1902 is GL_DEPTH_COMPONENT, 0x1406 is GL_FLOAT, 0x81A6 is GL_DEPTH_COMPONENT24
    gl::texImage2D(gl::cap::Texture2D, 0, 0x81A6, width, height, 0, 0x1902, 0x1406, nullptr);
    gl::texParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Nearest);
    gl::texParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Nearest);
    gl::texParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
    gl::texParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
    gl::framebufferTexture2D(gl::framebuffer::Framebuffer, 0x8D00 /* GL_DEPTH_ATTACHMENT */, gl::cap::Texture2D, depthTexture, 0);
  } else {
    gl::genRenderbuffers(1, &depthStencilRb);
    gl::bindRenderbuffer(gl::framebuffer::Renderbuffer, depthStencilRb);
    gl::renderbufferStorage(gl::framebuffer::Renderbuffer, 0x88F0 /* GL_DEPTH24_STENCIL8 */, width, height);
    gl::framebufferRenderbuffer(gl::framebuffer::Framebuffer, 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */, gl::framebuffer::Renderbuffer, depthStencilRb);
  }
  const bool ok = gl::checkFramebufferStatus(gl::framebuffer::Framebuffer) == gl::framebuffer::Complete;
  gl::bindRenderbuffer(gl::framebuffer::Renderbuffer, 0);
  gl::bindFramebuffer(gl::framebuffer::Framebuffer, 0);
  if(!ok) {
    destroy();
  }
  return ok;
}
bool Framebuffer::resize(int newWidth, int newHeight) {
  if(width == newWidth && height == newHeight) {
    return true;
  }
  int colorCount = static_cast<int>(colorTextures.size());
  bool useDepth = (depthTexture != 0);
  destroy();
  return initialize(newWidth, newHeight, colorCount, useDepth);
}
void Framebuffer::destroy() {
  if(depthStencilRb != 0) {
    gl::deleteRenderbuffers(1, &depthStencilRb);
    depthStencilRb = 0;
  }
  if(depthTexture != 0) {
    gl::deleteTextures(1, &depthTexture);
    depthTexture = 0;
  }
  if(!colorTextures.empty()) {
    gl::deleteTextures(static_cast<int>(colorTextures.size()), colorTextures.data());
    colorTextures.clear();
  }
  if(fbo != 0) {
    gl::deleteFramebuffers(1, &fbo);
    fbo = 0;
  }
  width = 0;
  height = 0;
}
void Framebuffer::bind() const {
  gl::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
}
void Framebuffer::unbind() {
  gl::bindFramebuffer(gl::framebuffer::Framebuffer, 0);
}
int FramebufferManager::create(int width, int height, int colorCount, bool useDepthTexture) {
  Framebuffer target;
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
  if(it == targets_.end() || attachmentIndex < 0 || attachmentIndex >= static_cast<int>(it->second.colorTextures.size())) {
    return -1;
  }
  return static_cast<int>(it->second.colorTextures[attachmentIndex]);
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
  Framebuffer::unbind();
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
                                       bool shadowEntities) {
  auto it = targets_.find(handle);
  if(it == targets_.end() || it->second.fbo == 0) {
    return false;
  }
  int prevFbo = 0;
  gl::getIntegerv(0x8CA6 /* GL_FRAMEBUFFER_BINDING */, &prevFbo);
  int prevViewport[4] = {0, 0, 0, 0};
  gl::getIntegerv(0x0BA2 /* GL_VIEWPORT */, prevViewport);
  const int prevRenderingHandle = renderingHandle_;
  FrameRenderCamera prevFrameCamera = renderer.frameCamera_;
  const FrameRenderCamera prevPublishedCamera = RenderCameraState::instance().frame();
  WorldRenderer* worldRenderer = renderer.client != nullptr ? renderer.client->worldRenderer.get() : nullptr;
  Entity* prevWorldCam = nullptr;
  bool prevRenderCamEntity = false;
  double prevFrameCamX = 0.0;
  double prevFrameCamY = 0.0;
  double prevFrameCamZ = 0.0;
  bool prevHasFrameCam = false;
  std::vector<std::pair<chunk::ChunkBuilder*, bool>> prevFrustumState;
  std::vector<std::vector<chunk::ChunkBuilder*>> prevVisibleDrawRings;
  if(worldRenderer != nullptr) {
    prevWorldCam = worldRenderer->cameraEntity_;
    prevRenderCamEntity = worldRenderer->renderCameraEntity_;
    prevFrameCamX = worldRenderer->frameCamX_;
    prevFrameCamY = worldRenderer->frameCamY_;
    prevFrameCamZ = worldRenderer->frameCamZ_;
    prevHasFrameCam = worldRenderer->hasFrameCamera_;
    prevVisibleDrawRings = worldRenderer->visibleDrawRings_;
    prevFrustumState.reserve(worldRenderer->sections_.size());
    for(auto& entry : worldRenderer->sections_) {
      prevFrustumState.emplace_back(entry.second.get(), entry.second->inFrustum);
    }
  }
  int prevChunkX = 0;
  int prevChunkZ = 0;
  LegacyChunkCache* legacyCache = nullptr;
  if(renderer.client != nullptr && renderer.client->world != nullptr) {
    if(auto* cs = renderer.client->world->getChunkSource()) {
      legacyCache = dynamic_cast<LegacyChunkCache*>(cs);
      if(legacyCache != nullptr) {
        prevChunkX = legacyCache->getCenterX();
        prevChunkZ = legacyCache->getCenterZ();
      }
    }
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
  const float clampedFov = std::clamp(fov, 1.0f, 179.0f);
  it->second.bind();
  renderingHandle_ = handle;
  renderer.renderToCurrentTarget(tickDelta, camera, clampedFov, it->second.width, it->second.height, true);
  renderingHandle_ = prevRenderingHandle;
  if(legacyCache != nullptr) {
    legacyCache->setChunkCacheCenter(prevChunkX, prevChunkZ);
  }
  if(worldRenderer != nullptr) {
    worldRenderer->cameraEntity_ = prevWorldCam;
    worldRenderer->renderCameraEntity_ = prevRenderCamEntity;
    worldRenderer->frameCamX_ = prevFrameCamX;
    worldRenderer->frameCamY_ = prevFrameCamY;
    worldRenderer->frameCamZ_ = prevFrameCamZ;
    worldRenderer->hasFrameCamera_ = prevHasFrameCam;
    worldRenderer->visibleDrawRings_ = std::move(prevVisibleDrawRings);
    for(const auto& [chunk, inFrustum] : prevFrustumState) {
      chunk->inFrustum = inFrustum;
    }
  }
  renderer.frameCamera_ = prevFrameCamera;
  RenderCameraState::instance().setFrame(prevPublishedCamera);
  gl::bindFramebuffer(gl::framebuffer::Framebuffer, prevFbo);
  gl::viewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
  return true;
}
} // namespace net::minecraft::client::render
