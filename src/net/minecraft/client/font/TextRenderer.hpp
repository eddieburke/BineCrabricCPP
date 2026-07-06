#pragma once
#include "net/minecraft/client/texture/TextureManager.hpp"
#include <array>
#include <memory>
#include <string>
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft::client::texture {
class TextureManager;
}
namespace net::minecraft::client::render {
class Tessellator;
}
namespace net::minecraft::client::font {
// Faithful port of net.minecraft.client.font.TextRenderer (beta 1.7.3).
class TextRenderer {
public:
  TextRenderer(option::GameOptions& options, const std::string& fontPath, texture::TextureManager& textureManager);
  TextRenderer(option::GameOptions& options, const texture::RasterImage& fontImage,
               texture::TextureManager& textureManager);
  static std::unique_ptr<TextRenderer> create(option::GameOptions& options, texture::TextureManager& textureManager,
                                              const std::string& fontPath);
  void drawWithShadow(const std::string& text, int x, int y, int color);
  void draw(const std::string& text, int x, int y, int color);
  void draw(const std::string& text, int x, int y, int color, bool shadow);
  [[nodiscard]] int getWidth(const std::string& text) const;
  void drawSplit(const std::string& text, int x, int y, int width, int color);
  [[nodiscard]] int splitAndGetHeight(const std::string& text, int width) const;
  int boundTexture = 0;

private:
  struct FontColor {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
  };
  void appendGlyphQuad(::net::minecraft::client::render::Tessellator& tessellator, int glyph, float penX, float r,
                       float g, float b, float a) const;
  std::array<int, 256> characterWidths_{};
  std::array<FontColor, 32> colors_{};
};
} // namespace net::minecraft::client::font
