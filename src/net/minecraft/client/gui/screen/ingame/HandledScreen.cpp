#include "net/minecraft/client/gui/screen/ingame/HandledScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
namespace net::minecraft::client::gui::screen::ingame {
namespace {
render::item::ItemRenderer itemRenderer;
} // namespace
void HandledScreen::init() {
  buttons_.clear();
  if(minecraft_ != nullptr && minecraft_->player != nullptr && container_ != nullptr) {
    minecraft_->player->currentScreenHandler = container_;
  }
}
void HandledScreen::render(int mouseX, int mouseY, float tickDelta) {
  if(minecraft_ == nullptr || textRenderer_ == nullptr || container_ == nullptr) {
    return;
  }
  renderBackground();
  const gl::preset::ContainerScreen screenCaps;
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  const int originX = (width_ - backgroundWidth) / 2;
  const int originY = (height_ - backgroundHeight) / 2;
  drawBackground(tickDelta);
  {
    gl::MatrixGuard lightingMatrix;
    gl::rotatef(120.0f, 1.0f, 0.0f, 0.0f);
    render::platform::Lighting::turnOn();
  }
  {
    const gl::preset::HandledSlotItem slotCaps;
    gl::MatrixGuard slotMatrix;
    gl::translatef(static_cast<float>(originX), static_cast<float>(originY), 0.0f);
    gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
    ::net::minecraft::screen::slot::Slot* hoveredSlot = nullptr;
    PlayerEntity& player = static_cast<PlayerEntity&>(*minecraft_->player);
    const ItemStack cursorStack = player.inventory.getCursorStack();
    for(::net::minecraft::screen::slot::Slot* slot : container_->slots) {
      if(slot == nullptr) {
        continue;
      }
      if(isPointOverSlot(*slot, mouseX, mouseY)) {
        hoveredSlot = slot;
        const gl::preset::SlotHighlight hoverCaps;
        fillGradient(slot->x, slot->y, slot->x + 16, slot->y + 16, 0x80FFFFFFU, 0x80FFFFFFU);
        gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
      }
      drawSlot(*slot);
    }
    if(!cursorStack.empty()) {
      gl::translatef(0.0f, 0.0f, 32.0f);
      itemRenderer.renderGuiItem(
          *textRenderer_, minecraft_->textureManager, cursorStack, mouseX - originX - 8, mouseY - originY - 8);
      itemRenderer.renderGuiItemDecoration(
          *textRenderer_, minecraft_->textureManager, cursorStack, mouseX - originX - 8, mouseY - originY - 8);
    }
    {
      const gl::preset::HandledSlotDecoration decorationCaps;
      render::platform::Lighting::turnOff();
      drawForeground();
      if(cursorStack.empty() && hoveredSlot != nullptr && hoveredSlot->hasStack()) {
        std::string label =
            resource::language::I18n::getClientTranslation(hoveredSlot->getStack().getTranslationKey());
        const auto trimChar = [](std::string& value, char ch) {
          while(!value.empty() && value.front() == ch) {
            value.erase(value.begin());
          }
          while(!value.empty() && value.back() == ch) {
            value.pop_back();
          }
        };
        trimChar(label, ' ');
        trimChar(label, '\t');
        if(!label.empty()) {
          const int tooltipX = mouseX - originX + 12;
          const int tooltipY = mouseY - originY - 12;
          const int textWidth = textRenderer_->getWidth(label);
          fillGradient(tooltipX - 3,
                       tooltipY - 3,
                       tooltipX + textWidth + 3,
                       tooltipY + 8 + 3,
                       0xC0000000U,
                       0xC0000000U);
          {
            const gl::preset::HandledTooltipDraw tooltipCaps;
            textRenderer_->drawWithShadow(label, tooltipX, tooltipY, 0xFFFFFF);
          }
          gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
      }
    }
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
void HandledScreen::drawForeground() {
}
int HandledScreen::containerOriginX() const noexcept {
  return (width_ - backgroundWidth) / 2;
}
int HandledScreen::containerOriginY() const noexcept {
  return (height_ - backgroundHeight) / 2;
}
void HandledScreen::drawContainerTexture(const char* texturePath, int srcU, int srcV, int drawW, int drawH) {
  if(minecraft_ == nullptr) {
    return;
  }
  const int textureId = minecraft_->textureManager.getTextureId(texturePath);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::bindTexture(gl::cap::Texture2D, textureId);
  drawTexture(containerOriginX(), containerOriginY(), srcU, srcV, drawW, drawH);
}
void HandledScreen::drawContainerTextureSplit(const char* texturePath, int topDrawH, int bottomSrcV, int bottomDrawH) {
  if(minecraft_ == nullptr) {
    return;
  }
  const int textureId = minecraft_->textureManager.getTextureId(texturePath);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::bindTexture(gl::cap::Texture2D, textureId);
  const int originX = containerOriginX();
  const int originY = containerOriginY();
  drawTexture(originX, originY, 0, 0, backgroundWidth, topDrawH);
  drawTexture(originX, originY + topDrawH, 0, bottomSrcV, backgroundWidth, bottomDrawH);
}
void HandledScreen::drawSlot(const ::net::minecraft::screen::slot::Slot& slot) {
  const ItemStack stack = slot.getStack();
  if(stack.empty()) {
    const int background = slot.getBackgroundTextureId();
    if(background >= 0) {
      const gl::CapScope slotBgCaps{gl::cap::Lighting};
      const gl::BoundTextureScope savedTexture;
      const gl::preset::ScreenTextOverlay slotOverlayCaps;
      gl::bindTexture(gl::cap::Texture2D, minecraft_->textureManager.getTextureId("/gui/items.png"));
      drawTexture(slot.x, slot.y, background % 16 * 16, background / 16 * 16, 16, 16);
    }
    return;
  }
  itemRenderer.renderGuiItem(*textRenderer_, minecraft_->textureManager, stack, slot.x, slot.y);
  itemRenderer.renderGuiItemDecoration(*textRenderer_, minecraft_->textureManager, stack, slot.x, slot.y);
}
::net::minecraft::screen::slot::Slot* HandledScreen::getSlotAt(int x, int y) {
  if(container_ == nullptr) {
    return nullptr;
  }
  for(::net::minecraft::screen::slot::Slot* slot : container_->slots) {
    if(slot != nullptr && isPointOverSlot(*slot, x, y)) {
      return slot;
    }
  }
  return nullptr;
}
bool HandledScreen::isPointOverSlot(const ::net::minecraft::screen::slot::Slot& slot, int x, int y) const {
  const int originX = (width_ - backgroundWidth) / 2;
  const int originY = (height_ - backgroundHeight) / 2;
  x -= originX;
  y -= originY;
  return x >= slot.x - 1 && x < slot.x + 16 + 1 && y >= slot.y - 1 && y < slot.y + 16 + 1;
}
void HandledScreen::mouseClicked(int mouseX, int mouseY, int button) {
  Screen::mouseClicked(mouseX, mouseY, button);
  if(minecraft_ == nullptr || minecraft_->player == nullptr || container_ == nullptr ||
     minecraft_->interactionManager == nullptr) {
    return;
  }
  if(button != 0 && button != 1) {
    return;
  }
  ::net::minecraft::screen::slot::Slot* slot = getSlotAt(mouseX, mouseY);
  const int originX = (width_ - backgroundWidth) / 2;
  const int originY = (height_ - backgroundHeight) / 2;
  const bool outside = mouseX < originX || mouseY < originY || mouseX >= originX + backgroundWidth ||
                       mouseY >= originY + backgroundHeight;
  int slotId = -1;
  if(slot != nullptr) {
    slotId = slot->id;
  }
  if(outside) {
    slotId = -999;
  }
  if(slotId == -1) {
    return;
  }
  const bool shift = input::InputSystem::instance().slotClickModifier() == input::SlotClickModifier::Shift;
  minecraft_->interactionManager->clickSlot(container_->syncId, slotId, button, shift, minecraft_->player);
}
void HandledScreen::keyPressed(char character, int keyCode) {
  (void)character;
  if(minecraft_ == nullptr || minecraft_->player == nullptr) {
    return;
  }
#ifdef _WIN32
  if(escapePressed(keyCode) || keyCode == static_cast<int>(minecraft_->options.inventoryKey.code)) {
    minecraft_->player->closeHandledScreen();
  }
#else
  if(escapePressed(keyCode)) {
    minecraft_->player->closeHandledScreen();
  }
#endif
}
void HandledScreen::removed() {
  if(minecraft_ != nullptr && minecraft_->interactionManager != nullptr && minecraft_->player != nullptr &&
     container_ != nullptr) {
    minecraft_->interactionManager->onScreenRemoved(container_->syncId, minecraft_->player);
  }
}
void HandledScreen::tick() {
  Screen::tick();
  if(minecraft_ != nullptr && minecraft_->player != nullptr) {
    if(!minecraft_->player->isAlive() || minecraft_->player->dead) {
      minecraft_->player->closeHandledScreen();
    }
  }
}
} // namespace net::minecraft::client::gui::screen::ingame
