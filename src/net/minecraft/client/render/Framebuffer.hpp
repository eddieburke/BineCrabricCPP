#pragma once
namespace net::minecraft::client::render {
enum class ColorFormat { Rgba8,
                         Rgba16F };
class Framebuffer {
 public:
 Framebuffer() = default;
 ~Framebuffer();
 Framebuffer(const Framebuffer&) = delete;
 Framebuffer& operator=(const Framebuffer&) = delete;
 bool initialize(int width, int height, ColorFormat format = ColorFormat::Rgba8);
 bool ensure(int width, int height, ColorFormat format = ColorFormat::Rgba8);
 void resize(int width, int height, ColorFormat format = ColorFormat::Rgba8);
 void begin();
 void end();
 void bindTarget();
 void blitToScreen(int screenWidth, int screenHeight);
 void destroy();
 [[nodiscard]] bool isValid() const {
  return valid_;
 }
  [[nodiscard]] int width() const {
   return width_;
  }
  [[nodiscard]] int height() const {
   return height_;
  }
  [[nodiscard]] ColorFormat format() const {
   return format_;
  }
  [[nodiscard]] unsigned int depthTexture() const {
   return depthStencilTexture_;
  }
  [[nodiscard]] unsigned int colorTexture() const {
   return colorTexture_;
  }

  private:
  bool allocate(int width, int height, ColorFormat format);
  unsigned int fbo_ = 0;
  unsigned int colorTexture_ = 0;
  unsigned int depthStencilTexture_ = 0;
 int width_ = 0;
 int height_ = 0;
 ColorFormat format_ = ColorFormat::Rgba8;
 int savedViewport_[4] = {0, 0, 0, 0};
 int previousBoundFbo_ = 0;
 bool valid_ = false;
 bool active_ = false;
};
} // namespace net::minecraft::client::render
