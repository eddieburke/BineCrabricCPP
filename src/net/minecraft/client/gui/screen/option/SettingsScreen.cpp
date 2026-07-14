#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include <algorithm>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
namespace net::minecraft::client::gui::screen::option {
SettingsScreen::SettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions, std::string title)
    : parentFactory_(std::move(parentFactory)), gameOptions_(gameOptions), title_(std::move(title)) {
}
void SettingsScreen::init() {
  buttons_.clear();
  if(gameOptions_ == nullptr || minecraft() == nullptr) {
    return;
  }
  OptionGuiBuilder gui(*this, *minecraft(), *gameOptions_);
  buildOptions(gui);
  refreshOptionStates();
  layout::OptionsBuildContext ctx{*this, *minecraft(), *gameOptions_};
  const std::size_t footerFirstButton = buttons_.size();
  int footerY = doneButtonY();
  publishScreenUi(mod::screen_regions::kFooter, &footerY);
  layout::addDoneButton(ctx, footerY, resource::language::I18n::getTranslation("gui.done"), [this] {
    if(gameOptions_ == nullptr || minecraft() == nullptr) {
      return;
    }
    gameOptions_->save();
    if(parentFactory_) {
      navigateTo(parentFactory_);
    }
  });
  int footerBottom = 0;
  for(std::size_t i = footerFirstButton; i < buttons_.size(); ++i) {
    if(buttons_[i] != nullptr && buttons_[i]->visible) {
      footerBottom = std::max(footerBottom, buttons_[i]->y + buttons_[i]->height);
    }
  }
  constexpr int kFooterBottomMargin = 8;
  const int overflow = footerBottom - (height() - kFooterBottomMargin);
  if(overflow > 0) {
    for(std::size_t i = footerFirstButton; i < buttons_.size(); ++i) {
      if(buttons_[i] != nullptr) {
        buttons_[i]->y -= overflow;
      }
    }
  }
}
void SettingsScreen::render(int mouseX, int mouseY, float tickDelta) {
  renderBackground();
  if(textRenderer() != nullptr) {
    drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 20, 0xFFFFFF);
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
void SettingsScreen::refreshOptionStates() {
  if(gameOptions_ == nullptr) {
    return;
  }
  layout::refreshOptionStates(buttons_, *gameOptions_);
  refreshOptionLabels(*this, *gameOptions_);
}
} // namespace net::minecraft::client::gui::screen::option
