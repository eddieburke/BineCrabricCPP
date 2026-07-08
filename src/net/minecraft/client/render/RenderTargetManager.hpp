#pragma once
#include <unordered_map>

namespace net::minecraft::client::render {
class GameRenderer;

struct RenderTarget {
    unsigned int fbo = 0;
    unsigned int colorTexture = 0;
    unsigned int depthStencilRb = 0;
    int width = 0;
    int height = 0;
    [[nodiscard]] bool initialize(int widthIn, int heightIn);
    void destroy();
    void bind() const;
    static void unbind();
};

class RenderTargetManager {
   public:
    [[nodiscard]] int create(int width, int height);
    bool destroy(int handle);
    [[nodiscard]] int textureId(int handle) const;
    bool resize(int handle, int newWidth, int newHeight);
    bool bind(int handle) const;
    static void unbind();
    [[nodiscard]] int width(int handle) const;
    [[nodiscard]] int height(int handle) const;
    bool render(int handle,
                GameRenderer& renderer,
                float tickDelta,
                double x,
                double y,
                double z,
                float yaw,
                float pitch,
                float roll,
                float fov);

    [[nodiscard]] int renderingHandle() const noexcept {
        return renderingHandle_;
    }

   private:
    std::unordered_map<int, RenderTarget> targets_{};
    int nextHandle_ = 1;
    int renderingHandle_ = -1;
};
}  // namespace net::minecraft::client::render
