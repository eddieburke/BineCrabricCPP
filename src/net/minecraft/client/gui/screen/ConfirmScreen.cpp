#include "net/minecraft/client/gui/screen/ConfirmScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
namespace net::minecraft::client::gui::screen {
ConfirmScreen::ConfirmScreen(ScreenFactory returnFactory, ConfirmHandler handler, std::string message1,
                             std::string message2, std::string confirmText, std::string cancelText)
    : returnFactory_(std::move(returnFactory)), handler_(std::move(handler)), message1_(std::move(message1)),
      message2_(std::move(message2)), confirmText_(std::move(confirmText)), cancelText_(std::move(cancelText)) {
}
void ConfirmScreen::init() {
  buttons_.clear();
  const int y = layout::confirmRowY(height());
  addActionButton(layout::confirmBtnX(width(), true), y, layout::kConfirmButtonWidth, layout::kDefaultButtonHeight,
                  confirmText_, [this] {
                    if(minecraft() == nullptr) {
                      return;
                    }
                    if(handler_) {
                      handler_(true);
                    }
                    navigateTo(returnFactory_);
                  });
  addActionButton(layout::confirmBtnX(width(), false), y, layout::kConfirmButtonWidth, layout::kDefaultButtonHeight,
                  cancelText_, [this] {
                    if(minecraft() == nullptr) {
                      return;
                    }
                    if(handler_) {
                      handler_(false);
                    }
                    navigateTo(returnFactory_);
                  });
}
void ConfirmScreen::render(int mouseX, int mouseY, float tickDelta) {
  renderBackground();
  if(textRenderer() != nullptr) {
    drawCenteredTextWithShadow(*textRenderer(), message1_, width() / 2, 70, 0xFFFFFF);
    drawCenteredTextWithShadow(*textRenderer(), message2_, width() / 2, 90, 0xFFFFFF);
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::gui::screen
