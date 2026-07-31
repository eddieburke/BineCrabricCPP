#pragma once
#include <array>
#include <memory>
#include <string>
#include "net/minecraft/client/texture/TextureManager.hpp"
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
 TextRenderer(option::GameOptions& options,
              const texture::RasterImage& fontImage,
              texture::TextureManager& textureManager);
 static std::unique_ptr<TextRenderer> create(option::GameOptions& options,
                                             texture::TextureManager& textureManager,
                                             const std::string& fontPath);
 void drawWithShadow(const std::string& text, int x, int y, int color);
 void drawCenteredWithShadow(const std::string& text, int x, int y, int color);
 // Scrolls long labels inside [minX,maxX] with a scissor clip (button titles, etc.).
 void drawClippedCenteredWithShadow(
     const std::string& text, int centerX, int y, int minX, int maxX, int color);
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
 void appendGlyphQuad(::net::minecraft::client::render::Tessellator& tessellator,
                      int glyph,
                      float penX,
                      float penY,
                      float r,
                      float g,
                      float b,
                      float a) const;
 // Appends one string's glyph quads into a running tessellation at the given
 // offset. Lets shadow + foreground share a single pass/draw call.
 void emitGlyphs(::net::minecraft::client::render::Tessellator& tessellator,
                 const std::string& text,
                 float offsetX,
                 float offsetY,
                 int color,
                 bool shadow,
                 float alpha) const;
 // Byte length of the longest prefix whose rendered width fits maxWidth.
 // Always consumes at least one glyph so word-wrap loops make progress.
 [[nodiscard]] std::size_t fitPrefixLength(const std::string& text, int maxWidth) const;
 std::array<int, 256> characterWidths_{};
 std::array<FontColor, 32> colors_{};
};
} // namespace net::minecraft::client::font
