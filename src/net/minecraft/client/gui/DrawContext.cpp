#include "net/minecraft/client/gui/DrawContext.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::gui {
namespace {
[[nodiscard]] int channel(std::uint32_t color, int shift) {
  return static_cast<int>((color >> static_cast<unsigned>(shift)) & 0xFFU);
}
[[nodiscard]] int rgb(std::uint32_t color) {
  return static_cast<int>(color & 0x00FFFFFFU);
}
} // namespace
void DrawContext::fill(int x1, int y1, int x2, int y2, std::uint32_t color) {
  if(x1 < x2) {
    std::swap(x1, x2);
  }
  if(y1 < y2) {
    std::swap(y1, y2);
  }
  const gl::AttribGuard attrib(gl::GL11::GL_ENABLE_BIT | gl::GL11::GL_CURRENT_BIT | gl::GL11::GL_TEXTURE_BIT);
  render::Tessellator& tessellator = render::INSTANCE;
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
  draw::coloredQuad(tessellator, x1, y1, x2, y2, rgb(color), channel(color, 24), zOffset);
}
void DrawContext::fillGradient(int x1, int y1, int x2, int y2, std::uint32_t colorStart, std::uint32_t colorEnd) {
  const gl::AttribGuard attrib(gl::GL11::GL_ENABLE_BIT | gl::GL11::GL_CURRENT_BIT | gl::GL11::GL_TEXTURE_BIT |
                               gl::GL11::GL_LIGHTING_BIT);
  render::Tessellator& tessellator = render::INSTANCE;
  gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glShadeModel(gl::GL11::GL_SMOOTH);
  draw::verticalGradientQuad(tessellator, x1, y1, x2, y2, rgb(colorStart), channel(colorStart, 24), rgb(colorEnd),
                             channel(colorEnd, 24), zOffset);
}
void DrawContext::drawTexture(int x, int y, int u, int v, int width, int height) {
  const gl::AttribGuard attrib(gl::GL11::GL_ENABLE_BIT | gl::GL11::GL_CURRENT_BIT | gl::GL11::GL_TEXTURE_BIT);
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  render::Tessellator& tessellator = render::INSTANCE;
  float currentColor[4]{1.0f, 1.0f, 1.0f, 1.0f};
  gl::GL11::glGetFloatv(gl::GL11::GL_CURRENT_COLOR, currentColor);
  tessellator.startQuads();
  tessellator.color(currentColor[0], currentColor[1], currentColor[2], currentColor[3]);
  draw::appendAtlasQuad(tessellator, x, y, u, v, width, height, zOffset);
  tessellator.draw();
}
void DrawContext::drawTextures(std::span<const draw::AtlasRect> rects) {
  if(rects.empty()) {
    return;
  }
  const gl::AttribGuard attrib(gl::GL11::GL_ENABLE_BIT | gl::GL11::GL_CURRENT_BIT | gl::GL11::GL_TEXTURE_BIT);
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  render::Tessellator& tessellator = render::INSTANCE;
  float currentColor[4]{1.0f, 1.0f, 1.0f, 1.0f};
  gl::GL11::glGetFloatv(gl::GL11::GL_CURRENT_COLOR, currentColor);
  tessellator.startQuads();
  tessellator.color(currentColor[0], currentColor[1], currentColor[2], currentColor[3]);
  for(const draw::AtlasRect& rect : rects) {
    draw::appendAtlasQuad(tessellator, rect.x, rect.y, rect.u, rect.v, rect.w, rect.h, zOffset);
  }
  tessellator.draw();
}
void DrawContext::drawCenteredTextWithShadow(font::TextRenderer& textRenderer, const std::string& text, int x, int y,
                                             int color) {
  textRenderer.drawWithShadow(text, x - textRenderer.getWidth(text) / 2, y, color);
}
void DrawContext::drawTextWithShadow(font::TextRenderer& textRenderer, const std::string& text, int x, int y, int color) {
  textRenderer.drawWithShadow(text, x, y, color);
}
} // namespace net::minecraft::client::gui
