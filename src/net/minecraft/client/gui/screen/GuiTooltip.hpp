#pragma once
#include <algorithm>
#include <string_view>
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::gui::screen {
inline void renderGuiTooltip(font::TextRenderer& textRenderer,
                             int screenWidth,
                             int screenHeight,
                             int mouseX,
                             int mouseY,
                             std::string_view description) {
 if(description.empty() || screenWidth < 24 || screenHeight < 18) {
  return;
 }
 const int textWidth = std::min(220, screenWidth - 16);
 const int bodyHeight = textRenderer.splitAndGetHeight(std::string(description), textWidth);
 const int boxWidth = textWidth + 8;
 const int boxHeight = bodyHeight + 8;
 int x = mouseX + 12;
 int y = mouseY + 12;
 if(x + boxWidth > screenWidth - 4) {
  x = mouseX - boxWidth - 12;
 }
 if(y + boxHeight > screenHeight - 4) {
  y = mouseY - boxHeight - 12;
 }
 x = std::clamp(x, 4, screenWidth - boxWidth - 4);
 y = std::clamp(y, 4, screenHeight - boxHeight - 4);
 {
  const render::RenderPassScope passScope(render::RenderType::gui());
  draw::verticalGradientQuad(render::Tessellator::INSTANCE,
                             x,
                             y,
                             x + boxWidth,
                             y + boxHeight,
                             0x101010,
                             0xE8,
                             0x101010,
                             0xE8);
 }
 textRenderer.drawSplit(std::string(description), x + 4, y + 4, textWidth, 0xFFFFFFFF);
}
}
