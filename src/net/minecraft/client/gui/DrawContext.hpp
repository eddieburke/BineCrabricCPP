#pragma once
#include <cstdint>
#include <span>
#include <string>
namespace net::minecraft::client::font {
class TextRenderer;
}
namespace net::minecraft::client::gui::draw {
struct AtlasRect;
}
namespace net::minecraft::client::gui {
// Faithful port of net.minecraft.client.gui.DrawContext (beta 1.7.3).
class DrawContext {
public:
  virtual ~DrawContext() = default;
  void fill(int x1, int y1, int x2, int y2, std::uint32_t color);
  void fillGradient(int x1, int y1, int x2, int y2, std::uint32_t colorStart, std::uint32_t colorEnd);
  void drawTexture(int x, int y, int u, int v, int width, int height);
  void drawTextures(std::span<const draw::AtlasRect> rects);
  void drawCenteredTextWithShadow(font::TextRenderer& textRenderer, const std::string& text, int x, int y, int color);
  void drawTextWithShadow(font::TextRenderer& textRenderer, const std::string& text, int x, int y, int color);
  float zOffset = 0.0f;
};
} // namespace net::minecraft::client::gui
