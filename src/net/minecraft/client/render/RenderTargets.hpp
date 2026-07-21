#pragma once
#include <unordered_map>
#include <vector>
namespace net::minecraft::client::render {
class GameRenderer;
struct RenderTarget {
 unsigned int fbo = 0;
 std::vector<unsigned int> colorTextures{};
 unsigned int depthStencilRb = 0;
 unsigned int depthTexture = 0;
 int width = 0;
 int height = 0;
 [[nodiscard]] bool initialize(int widthIn, int heightIn, int colorCount = 1, bool useDepthTexture = false);
 bool resize(int newWidth, int newHeight);
 void destroy();
 void bind() const;
 static void unbind();
};
class FramebufferManager {
 public:
 [[nodiscard]] int create(int width, int height, int colorCount = 1, bool useDepthTexture = false);
 bool destroy(int handle);
 [[nodiscard]] int textureId(int handle, int attachmentIndex = 0) const;
 [[nodiscard]] int depthTextureId(int handle) const;
 bool resize(int handle, int newWidth, int newHeight);
 bool bind(int handle) const;
 static void unbind();
 [[nodiscard]] int width(int handle) const;
 [[nodiscard]] int height(int handle) const;
 bool renderWorldTo(int handle,
                    GameRenderer& renderer,
                    float tickDelta,
                    double x,
                    double y,
                    double z,
                    float yaw,
                    float pitch,
                    float roll,
                    float fov,
                    bool orthographic = false,
                    float orthoHalfWidth = 1.0f,
                    float orthoHalfHeight = 1.0f,
                    float orthoNear = -1.0f,
                    float orthoFar = 1.0f,
                    bool shadowPass = false,
                    bool shadowEntities = true,
                    float perspectiveNear = 0.0f,
                    float perspectiveFar = 0.0f);
 [[nodiscard]] int renderingHandle() const noexcept {
  return renderingHandle_;
 }

 private:
 std::unordered_map<int, RenderTarget> targets_{};
 int nextHandle_ = 1;
 int renderingHandle_ = -1;
};
} // namespace net::minecraft::client::render
