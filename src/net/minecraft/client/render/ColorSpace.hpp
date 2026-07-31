#pragma once

#include <memory>

namespace net::minecraft::client::gl {
class ShaderProgram;
}

namespace net::minecraft::client::render {

enum class ColorSpace : int {
 Srgb = 0,
 DciP3 = 1,
 DisplayP3 = 2,
 Rec2020 = 3,
 AdobeRgb = 4,
};

[[nodiscard]] inline ColorSpace colorSpaceFromOption(int value) noexcept {
 return (value >= 0 && value <= 4) ? static_cast<ColorSpace>(value) : ColorSpace::Srgb;
}

// https://shaders.properties/current/reference/shadersproperties/features/
class ColorSpaceConverter {
 public:
 ColorSpaceConverter() = default;
 ~ColorSpaceConverter();
 ColorSpaceConverter(const ColorSpaceConverter&) = delete;
 ColorSpaceConverter& operator=(const ColorSpaceConverter&) = delete;

 void destroy();
 void rebuild(int width, int height, ColorSpace space);
 void process(unsigned int targetTexture);

 [[nodiscard]] unsigned int writeFramebuffer() const noexcept { return presentFbo_; }
 [[nodiscard]] unsigned int presentTexture() const noexcept { return presentTexture_; }
 [[nodiscard]] bool ready() const noexcept;

 bool blitPresentToScreen(int screenWidth, int screenHeight);

 private:
 bool ensureTargets(int width, int height);
 bool ensureProgram(ColorSpace space);

 int width_ = 0;
 int height_ = 0;
 ColorSpace space_ = ColorSpace::Srgb;
 unsigned int presentTexture_ = 0;
 unsigned int presentFbo_ = 0;
 unsigned int swapTexture_ = 0;
 unsigned int swapFbo_ = 0;
 std::unique_ptr<gl::ShaderProgram> program_;
};

}
