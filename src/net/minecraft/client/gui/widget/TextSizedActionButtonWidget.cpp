#include "net/minecraft/client/gui/widget/TextSizedActionButtonWidget.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::gui::widget {
int TextSizedActionButtonWidget::measureWidth(const font::TextRenderer& textRenderer, const std::string& text) {
  return textRenderer.getWidth(text) + kHorizontalPad + kLeftCrop;
}
void TextSizedActionButtonWidget::render(client::Minecraft& minecraft, font::TextRenderer& textRenderer, int mouseX,
                                         int mouseY) {
  if(!visible) {
    return;
  }
  const int textureId = minecraft.textureManager.getTextureId("/gui/gui.png");
  gl::bindTexture(gl::cap::Texture2D, textureId);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  const bool hovered = mouseX >= x && mouseY >= y && mouseX < x + width && mouseY < y + height;
  const int imageY = getYImage(hovered);
  drawTexture(x, y, kLeftCrop, 46 + imageY * 20, width / 2, height);
  drawTexture(x + width / 2, y, 200 - width / 2, 46 + imageY * 20, width / 2, height);
  renderBackground(mouseX, mouseY);
  const int textY = y + (height - 8) / 2;
  if(!active) {
    drawCenteredTextWithShadow(textRenderer, text, x + width / 2, textY, 0xFFA0A0A0);
  } else if(hovered) {
    drawCenteredTextWithShadow(textRenderer, text, x + width / 2, textY, 0xFFFFA0);
  } else {
    drawCenteredTextWithShadow(textRenderer, text, x + width / 2, textY, 0xFFE0E0E0);
  }
}
} // namespace net::minecraft::client::gui::widget
