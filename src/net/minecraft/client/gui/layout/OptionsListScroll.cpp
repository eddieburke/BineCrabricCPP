#include "net/minecraft/client/gui/layout/OptionsListScroll.hpp"
#include <algorithm>
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::gui::layout {
void OptionsListScroll::renderFooterDim(int screenWidth, int screenHeight) const {
 int x1 = 0;
 int y1 = listBottom_;
 int x2 = screenWidth;
 int y2 = screenHeight;
 if(x1 < x2) {
  std::swap(x1, x2);
 }
 if(y1 < y2) {
  std::swap(y1, y2);
 }
 constexpr std::uint32_t color = 0xE0101010U;
 const render::RenderPassScope passScope(render::RenderType::gui());
 draw::coloredQuad(render::INSTANCE,
                   x1,
                   y1,
                   x2,
                   y2,
                   static_cast<int>(color & 0x00FFFFFFU),
                   static_cast<int>((color >> 24U) & 0xFFU));
}
void OptionsListScroll::renderScrollbar(int screenWidth) const {
 if(maxScroll_ <= 0) {
  return;
 }
 const int trackX = scrollbarTrackX(screenWidth);
 const int trackHeight = listBottom_ - listTop_;
 const int thumbHeight = std::max(16, trackHeight * trackHeight / std::max(trackHeight, contentHeight_));
 const int thumbTravel = std::max(0, trackHeight - thumbHeight);
 const int thumbY = listTop_ + (maxScroll_ == 0 ? 0 : scrollOffset_ * thumbTravel / maxScroll_);
 const render::RenderPassScope passScope(render::RenderType::gui());
 draw::coloredQuad(render::INSTANCE, trackX + 2, listBottom_, trackX, listTop_, 0x000000, 0x60);
 draw::coloredQuad(render::INSTANCE, trackX + 2, thumbY + thumbHeight, trackX, thumbY, 0xC0C0C0, 0xFF);
}
void OptionsListScroll::renderHeaders(font::TextRenderer* textRenderer, int screenWidth) const {
 if(textRenderer == nullptr) {
  return;
 }
 for(const Header& header : headers_) {
  const int y = listTop_ + header.contentY - scrollOffset_;
  if(y + 8 > listTop_ && y < listBottom_) {
   textRenderer->drawWithShadow(header.text, twoColumnLeftX(screenWidth), y, 0xFFFFA0);
  }
 }
}
} // namespace net::minecraft::client::gui::layout
