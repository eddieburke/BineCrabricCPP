#include "net/minecraft/client/gui/DrawContext.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
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
void DrawContext::drawClippedCenteredTextWithShadow(
    font::TextRenderer& textRenderer, const std::string& text, int centerX, int y, int minX, int maxX, int color) {
 const int textWidth = textRenderer.getWidth(text);
 const int availableWidth = maxX - minX;
 if(textWidth <= availableWidth) {
  textRenderer.drawWithShadow(text, centerX - textWidth / 2, y, color);
  return;
 }
 const int maxScroll = textWidth - availableWidth;
 const std::uint64_t nowMs =
     std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
 const double cycleMs = 4000.0;
 double phase = std::fmod(static_cast<double>(nowMs), cycleMs) / cycleMs;
 double offsetFraction = 0.0;
 if(phase < 0.2) {
  offsetFraction = 0.0;
 } else if(phase < 0.7) {
  offsetFraction = (phase - 0.2) / 0.5;
 } else if(phase < 0.8) {
  offsetFraction = 1.0;
 } else {
  offsetFraction = 1.0 - (phase - 0.8) / 0.2;
 }
 const int textX = minX - static_cast<int>(offsetFraction * maxScroll);

 const render::RenderSystem::StateShadow shadow = RenderSystem::getShadow();
 RenderSystem::StateShadow scissorState = shadow;
 scissorState.blend = true;

 int origViewport[4] = {0, 0, 800, 600};
 RenderSystem::getCachedViewport(origViewport);
 const int scissorX = std::max(0, minX);
 const int scissorY = std::max(0, origViewport[3] - (y + 10));
 const int scissorW = std::max(0, maxX - minX);
 const int scissorH = 12;

 ::glEnable(net::minecraft::client::gl::cap::ScissorTest);
 ::glScissor(scissorX, scissorY, scissorW, scissorH);
 textRenderer.drawWithShadow(text, textX, y, color);
 ::glDisable(net::minecraft::client::gl::cap::ScissorTest);

 RenderSystem::setShadow(shadow);
}
} // namespace net::minecraft::client::gui
