#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include <algorithm>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace {
constexpr int kListTop = 32;
constexpr int kFooterGap = 8;
} // namespace
SettingsScreen::SettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions, std::string title)
    : parentFactory_(std::move(parentFactory)), gameOptions_(gameOptions), title_(std::move(title)) {
}
void SettingsScreen::init() {
 buttons_.clear();
 optionButtonCount_ = 0;
 scroll_.clear();
 if(gameOptions_ == nullptr || minecraft() == nullptr) {
  return;
 }
 OptionGuiBuilder gui(*this, *minecraft(), *gameOptions_);
 buildOptions(gui);
 optionButtonCount_ = buttons_.size();
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
 int footerTop = footerY;
 for(std::size_t i = footerFirstButton; i < buttons_.size(); ++i) {
  if(buttons_[i] != nullptr && buttons_[i]->visible) {
   footerTop = std::min(footerTop, buttons_[i]->y);
  }
 }
 scroll_.setViewport(kListTop, std::max(kListTop, footerTop - kFooterGap));
 scroll_.captureButtons(buttons_, optionButtonCount_);
 updateScrollLayout();
}
void SettingsScreen::updateScrollLayout() {
 scroll_.apply(buttons_);
}
void SettingsScreen::render(int mouseX, int mouseY, float tickDelta) {
 if(scroll_.draggingThumb()) {
  scroll_.mouseDragged(mouseY);
  updateScrollLayout();
 }
 renderBackground();
 scroll_.renderFooterDim(width(), height());
 if(textRenderer() != nullptr) {
  textRenderer()->drawCenteredWithShadow(title_, width() / 2, 12, 0xFFFFFF);
 }
 scroll_.renderHeaders(textRenderer(), width());
 scroll_.renderScrollbar(width());
 Screen::render(mouseX, mouseY, tickDelta);
}
void SettingsScreen::mouseScrolled(int mouseX, int mouseY, int delta) {
 (void)mouseX;
 if(scroll_.mouseScrolled(mouseY, delta)) {
  updateScrollLayout();
 }
}
void SettingsScreen::mouseClicked(int mouseX, int mouseY, int button) {
 if(button == 0 && scroll_.mouseClicked(mouseX, mouseY, width())) {
  updateScrollLayout();
  return;
 }
 Screen::mouseClicked(mouseX, mouseY, button);
}
void SettingsScreen::mouseReleased(int mouseX, int mouseY, int button) {
 scroll_.mouseReleased();
 Screen::mouseReleased(mouseX, mouseY, button);
}
void SettingsScreen::keyPressed(char character, int keyCode) {
 if(arrowUpPressed(keyCode)) {
  scroll_.scrollBy(-layout::OptionsListScroll::kScrollStep);
  updateScrollLayout();
  return;
 }
 if(arrowDownPressed(keyCode)) {
  scroll_.scrollBy(layout::OptionsListScroll::kScrollStep);
  updateScrollLayout();
  return;
 }
 if(escapePressed(keyCode)) {
  if(gameOptions_ != nullptr) {
   gameOptions_->save();
  }
  if(parentFactory_) {
   navigateTo(parentFactory_);
  } else {
   closeScreen();
  }
  return;
 }
 Screen::keyPressed(character, keyCode);
}
void SettingsScreen::refreshOptionStates() {
 if(gameOptions_ == nullptr) {
  return;
 }
 layout::refreshOptionStates(buttons_, *gameOptions_);
 refreshOptionLabels(*this, *gameOptions_);
}
} // namespace net::minecraft::client::gui::screen::option
