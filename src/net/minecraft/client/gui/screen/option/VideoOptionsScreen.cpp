#include "net/minecraft/client/gui/screen/option/VideoOptionsScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/option/AnimationSettingsScreen.hpp"
#include "net/minecraft/client/gui/screen/option/DetailSettingsScreen.hpp"
#include "net/minecraft/client/gui/screen/option/PerformanceSettingsScreen.hpp"
#include "net/minecraft/client/gui/screen/option/QualitySettingsScreen.hpp"
#include "net/minecraft/client/gui/screen/option/WorldSettingsScreen.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
namespace net::minecraft::client::gui::screen::option {
VideoOptionsScreen::VideoOptionsScreen(ParentFactory parentFactory, client_option::GameOptions* options)
    : parentFactory_(std::move(parentFactory)), options_(options) {
}
void VideoOptionsScreen::init() {
  title_ = resource::language::I18n::getTranslation("options.videoTitle");
  buttons_.clear();
  if(options_ == nullptr || minecraft() == nullptr) {
    return;
  }
  const int startY = height() / 6;
  const int x1 = layout::optionsGridX(width(), 0);
  const int x2 = layout::optionsGridX(width(), 1);
  const auto returnHere = [factory = parentFactory_, options = options_]() {
    return std::make_unique<VideoOptionsScreen>(factory, options);
  };
  addActionButton(x1, startY, 310, layout::kDefaultButtonHeight, "Quality...", [this, returnHere] {
    options_->save();
    navigateTo([returnHere, opts = options_]() { return makeQualitySettingsScreen(returnHere, opts); });
  });
  addActionButton(
      x1, startY + layout::kRowSpacing, 150, layout::kDefaultButtonHeight, "Performance...", [this, returnHere] {
        options_->save();
        navigateTo([returnHere, opts = options_]() { return makePerformanceSettingsScreen(returnHere, opts); });
      });
  addActionButton(
      x2, startY + layout::kRowSpacing, 150, layout::kDefaultButtonHeight, "Details...", [this, returnHere] {
        options_->save();
        navigateTo([returnHere, opts = options_]() { return makeDetailSettingsScreen(returnHere, opts); });
      });
  addActionButton(
      x1, startY + layout::kRowSpacing * 2, 150, layout::kDefaultButtonHeight, "Animations...", [this, returnHere] {
        options_->save();
        navigateTo([returnHere, opts = options_]() { return makeAnimationSettingsScreen(returnHere, opts); });
      });
  addActionButton(
      x2, startY + layout::kRowSpacing * 2, 150, layout::kDefaultButtonHeight, "World...", [this, returnHere] {
        options_->save();
        navigateTo(
            [returnHere, opts = options_]() { return std::make_unique<WorldSettingsScreen>(returnHere, opts); });
      });
  addCenteredActionButton(
      startY + layout::kRowSpacing * 3, resource::language::I18n::getTranslation("gui.done"), [this] {
        if(options_ == nullptr || minecraft() == nullptr) {
          return;
        }
        options_->save();
        navigateTo(parentFactory_);
      });
}
void VideoOptionsScreen::render(int mouseX, int mouseY, float tickDelta) {
  renderBackground();
  if(textRenderer() != nullptr) {
    drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 20, 0xFFFFFF);
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::gui::screen::option
