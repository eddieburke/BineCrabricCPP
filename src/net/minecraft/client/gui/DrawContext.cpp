#include "net/minecraft/client/gui/DrawContext.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::gui {
namespace {
using net::minecraft::client::render::RenderSystem;
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
 render::Tessellator& tessellator = render::INSTANCE;
 const render::RenderPassScope passScope(render::RenderType::gui());
 draw::coloredQuad(tessellator, x1, y1, x2, y2, rgb(color), channel(color, 24), zOffset);
}
void DrawContext::fillGradient(int x1, int y1, int x2, int y2, std::uint32_t colorStart, std::uint32_t colorEnd) {
 render::Tessellator& tessellator = render::INSTANCE;
 const render::RenderPassScope passScope(render::RenderType::gui());
 draw::verticalGradientQuad(tessellator,
                            x1,
                            y1,
                            x2,
                            y2,
                            rgb(colorStart),
                            channel(colorStart, 24),
                            rgb(colorEnd),
                            channel(colorEnd, 24),
                            zOffset);
}
void DrawContext::drawTexture(int x, int y, int u, int v, int width, int height) {
 const render::RenderPassScope passScope(render::RenderType::guiTextured());
 const RenderSystem::StateShadow shadow = RenderSystem::getShadow();
 render::Tessellator& tessellator = render::INSTANCE;
 tessellator.startQuads();
 tessellator.color(shadow.constColor[0], shadow.constColor[1], shadow.constColor[2], shadow.constColor[3]);
 draw::appendAtlasQuad(tessellator, x, y, u, v, width, height, zOffset);
 tessellator.draw();
}
void DrawContext::drawTextures(std::span<const draw::AtlasRect> rects) {
 if(rects.empty()) {
  return;
 }
 const render::RenderPassScope passScope(render::RenderType::guiTextured());
 const RenderSystem::StateShadow shadow = RenderSystem::getShadow();
 render::Tessellator& tessellator = render::INSTANCE;
 tessellator.startQuads();
 tessellator.color(shadow.constColor[0], shadow.constColor[1], shadow.constColor[2], shadow.constColor[3]);
 for(const draw::AtlasRect& rect : rects) {
  draw::appendAtlasQuad(tessellator, rect.x, rect.y, rect.u, rect.v, rect.w, rect.h, zOffset);
 }
 tessellator.draw();
}
void DrawContext::drawCenteredTextWithShadow(
    font::TextRenderer& textRenderer, const std::string& text, int x, int y, int color) {
 textRenderer.drawWithShadow(text, x - textRenderer.getWidth(text) / 2, y, color);
}
void DrawContext::drawTextWithShadow(
    font::TextRenderer& textRenderer, const std::string& text, int x, int y, int color) {
 textRenderer.drawWithShadow(text, x, y, color);
}
} // namespace net::minecraft::client::gui
