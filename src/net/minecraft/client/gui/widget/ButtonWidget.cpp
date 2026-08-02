#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::gui::widget {
namespace core = net::minecraft::client::render::core;
ButtonWidget::ButtonWidget(int idIn, int xIn, int yIn, int widthIn, int heightIn, std::string textIn)
    : width(widthIn), height(heightIn), x(xIn), y(yIn), text(std::move(textIn)), id(idIn) {
}
int ButtonWidget::getYImage(bool hovered) const {
 int result = 1;
 if(!active) {
  result = 0;
 } else if(hovered) {
  result = 2;
 }
 return result;
}
void ButtonWidget::render(client::Minecraft& minecraft, font::TextRenderer& textRenderer, int mouseX, int mouseY) {
 if(!visible) {
  return;
 }
 const int textureId = minecraft.textureManager.getTextureId("/gui/gui.png");
 minecraft.textureManager.bindTexture(textureId);
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 // Inline bounds check for hover rendering only; isMouseOver() is reserved for
 // click handling (SliderWidget overrides it to begin dragging on mouse down).
 const bool hovered = mouseX >= x && mouseY >= y && mouseX < x + width && mouseY < y + height;
 const int imageY = getYImage(hovered);
 {
  const render::RenderPassScope passScope(render::RenderType::guiTextured());
  const float* c = core::constColor();
  render::Tessellator& tess = render::INSTANCE;
  tess.startQuads();
  tess.color(c[0], c[1], c[2], c[3]);
  draw::appendAtlasQuad(tess, x, y, 0, 46 + imageY * 20, width / 2, height, 0.0f);
  draw::appendAtlasQuad(tess, x + width / 2, y, 200 - width / 2, 46 + imageY * 20, width / 2, height, 0.0f);
  tess.draw();
 }
 renderBackground(mouseX, mouseY);
 const int textY = y + (height - 8) / 2;
 const int minX = x + 4;
 const int maxX = x + width - 4;
 const int textColor = !active ? 0xFFA0A0A0 : (hovered ? 0xFFFFA0 : 0xFFE0E0E0);
 textRenderer.drawClippedCenteredWithShadow(text, x + width / 2, textY, minX, maxX, textColor);
}
void ButtonWidget::renderBackground(int mouseX, int mouseY) {
 (void)mouseX;
 (void)mouseY;
}
void ButtonWidget::onMouseDown(int mouseX, int mouseY) {
 (void)mouseX;
 (void)mouseY;
}
void ButtonWidget::mouseReleased(int mouseX, int mouseY) {
 (void)mouseX;
 (void)mouseY;
}
bool ButtonWidget::isMouseOver(int mouseX, int mouseY) const {
 // Java GuiButton.mousePressed() gates on enabled *and* visible. Hidden widgets keep
 // their bounds (scrolling lists park off-screen rows over the footer), so dropping the
 // visible check lets them swallow clicks aimed at the buttons drawn on top of them.
 return visible && active && mouseX >= x && mouseY >= y && mouseX < x + width && mouseY < y + height;
}
} // namespace net::minecraft::client::gui::widget
