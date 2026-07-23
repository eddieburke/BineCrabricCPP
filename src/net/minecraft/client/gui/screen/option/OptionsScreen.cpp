#include "net/minecraft/client/gui/screen/option/OptionsScreen.hpp"
#include <array>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/screen/GameMenuScreen.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/gui/screen/option/KeybindsScreen.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/VideoOptionsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
namespace net::minecraft::client::gui::screen::option {
using client_option::GameOptions;
namespace options_screen {
namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::OptionSpec;
std::array<OptionSpec, 8> kSpecs{{
    d::makeSlider("music",
                  0,
                  ApplyFlags::None,
                  ApplyFlags::UpdateSound,
                  d::getFloatMember<&GameOptions::musicVolume>,
                  d::setFloatMember<&GameOptions::musicVolume>,
                  d::loadFloatMember<&GameOptions::musicVolume>,
                  d::saveFloatMember<&GameOptions::musicVolume>),
    d::makeSlider("sound",
                  1,
                  ApplyFlags::None,
                  ApplyFlags::UpdateSound,
                  d::getFloatMember<&GameOptions::soundVolume>,
                  d::setFloatMember<&GameOptions::soundVolume>,
                  d::loadFloatMember<&GameOptions::soundVolume>,
                  d::saveFloatMember<&GameOptions::soundVolume>),
    d::makeToggle("invertYMouse",
                  2,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::invertYMouse>,
                  d::cycleBoolMember<&GameOptions::invertYMouse>,
                  d::loadBoolMember<&GameOptions::invertYMouse>,
                  d::saveBoolMember<&GameOptions::invertYMouse>),
    d::makeSlider("mouseSensitivity",
                  3,
                  ApplyFlags::None,
                  ApplyFlags::None,
                  d::getFloatMember<&GameOptions::mouseSensitivity>,
                  d::setFloatMember<&GameOptions::mouseSensitivity>,
                  d::loadFloatMember<&GameOptions::mouseSensitivity>,
                  d::saveFloatMember<&GameOptions::mouseSensitivity>),
    d::makeCycle("difficulty",
                 10,
                 ApplyFlags::None,
                 d::cycleIntMod<&GameOptions::difficulty, 4>,
                 d::loadIntMember<&GameOptions::difficulty>,
                 d::saveIntMember<&GameOptions::difficulty>),
    d::makeSlider("fov",
                  14,
                  ApplyFlags::None,
                  ApplyFlags::None,
                  d::getFloatMember<&GameOptions::fieldOfView>,
                  d::setFloatMember<&GameOptions::fieldOfView>,
                  d::loadFloatMember<&GameOptions::fieldOfView>,
                  d::saveFloatMember<&GameOptions::fieldOfView>),
    d::makeToggle("bobView",
                  6,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::bobView>,
                  d::cycleBoolMember<&GameOptions::bobView>,
                  d::loadBoolMember<&GameOptions::bobView>,
                  d::saveBoolMember<&GameOptions::bobView>),
    d::makeCycle("guiScale",
                 5,
                 ApplyFlags::None,
                 d::cycleIntMod<&GameOptions::guiScale, 4>,
                 d::loadIntMember<&GameOptions::guiScale>,
                 d::saveIntMember<&GameOptions::guiScale>,
                 nullptr,
                 nullptr,
                 d::alwaysResize),
}};
} // namespace options_screen
OptionsScreen::OptionsScreen(screen::ScreenFactory parentFactory, net::minecraft::client::option::GameOptions* options)
    : parentFactory_(std::move(parentFactory)), options_(options) {
}
void OptionsScreen::init() {
 title_ = resource::language::I18n::getTranslation("options.title");
 buttons_.clear();
 if(options_ == nullptr || minecraft() == nullptr) {
  return;
 }
 if(!parentFactory_) {
  if(minecraft()->world != nullptr) {
   parentFactory_ = []() { return std::make_unique<GameMenuScreen>(); };
  } else {
   parentFactory_ = []() { return std::make_unique<TitleScreen>(); };
  }
 }
 OptionGuiBuilder gui(*this, *minecraft(), *options_);
 const int x1 = gui.gridX(0);
 const int x2 = gui.gridX(1);
 const int y0 = gui.gridY(0);
 constexpr int dy = layout::kRowSpacing;
 const std::string music = resource::language::I18n::getTranslation("options.music");
 const std::string sound = resource::language::I18n::getTranslation("options.sound");
 const std::string sensitivity = resource::language::I18n::getTranslation("options.sensitivity");
 const std::string difficulty = resource::language::I18n::getTranslation("options.difficulty");
 const std::string guiScale = resource::language::I18n::getTranslation("options.guiScale");
 const std::string bobView = resource::language::I18n::getTranslation("options.viewBobbing");
 const std::string invertMouse = resource::language::I18n::getTranslation("options.invertMouse");
 gui.slider(x1, y0, "music", music.c_str(), [music](const GameOptions& o) {
  return percentLabel(music.c_str(), o.musicVolume);
 });
 gui.slider(x2, y0, "sound", sound.c_str(), [sound](const GameOptions& o) {
  return percentLabel(sound.c_str(), o.soundVolume);
 });
 gui.toggle(x1, y0 + dy, "invertYMouse", invertMouse.c_str());
 gui.slider(x2, y0 + dy, "mouseSensitivity", sensitivity.c_str(), [sensitivity](const GameOptions& o) {
  if(o.mouseSensitivity == 0.0f) {
   return optionLabel(sensitivity.c_str(),
                      resource::language::I18n::getTranslation("options.sensitivity.min"));
  }
  if(o.mouseSensitivity == 1.0f) {
   return optionLabel(sensitivity.c_str(),
                      resource::language::I18n::getTranslation("options.sensitivity.max"));
  }
  return optionLabel(sensitivity.c_str(), std::to_string(static_cast<int>(o.mouseSensitivity * 200.0f)) + "%");
 });
 gui.i18nCycle(x1,
               y0 + dy * 2,
               "difficulty",
               difficulty.c_str(),
               {"options.difficulty.peaceful",
                "options.difficulty.easy",
                "options.difficulty.normal",
                "options.difficulty.hard"},
               &GameOptions::difficulty);
 gui.slider(x2, y0 + dy * 2, "fov", "FOV", [](const GameOptions& o) {
  if(o.fieldOfView == 0.0f) {
   return optionLabel("FOV", "Normal");
  }
  if(o.fieldOfView == 1.0f) {
   return optionLabel("FOV", "Quake Pro");
  }
  return optionLabel("FOV", std::to_string(static_cast<int>(70.0f + o.fieldOfView * 40.0f)));
 });
 gui.toggle(x1, y0 + dy * 3, "bobView", bobView.c_str());
 gui.i18nCycle(
     x2,
     y0 + dy * 3,
     "guiScale",
     guiScale.c_str(),
     {"options.guiScale.auto", "options.guiScale.small", "options.guiScale.normal", "options.guiScale.large"},
     &GameOptions::guiScale);
 layout::refreshOptionStates(buttons_, *options_);
 const int navY = layout::optionsGridY(height(), 4);
 const auto returnHere = [factory = parentFactory_, options = options_]() {
  return std::make_unique<OptionsScreen>(factory, options);
 };
 addCenteredActionButton(navY,
                         layout::kDefaultButtonWidth,
                         layout::kDefaultButtonHeight,
                         resource::language::I18n::getTranslation("options.video"),
                         [this, returnHere] {
                          options_->save();
                          navigateTo([returnHere, opts = options_]() {
                           return std::make_unique<VideoOptionsScreen>(returnHere, opts);
                          });
                         });
 addCenteredActionButton(
     navY + layout::kRowSpacing,
     layout::kDefaultButtonWidth,
     layout::kDefaultButtonHeight,
     resource::language::I18n::getTranslation("options.controls"),
     [this, returnHere] {
      options_->save();
      navigateTo([returnHere, opts = options_]() { return std::make_unique<KeybindsScreen>(returnHere, opts); });
     });
 layout::OptionsBuildContext ctx{*this, *minecraft(), *options_};
 layout::addDoneButton(
     ctx, navY + layout::kRowSpacing * 3, resource::language::I18n::getTranslation("gui.done"), [this] {
      if(options_ == nullptr || minecraft() == nullptr) {
       return;
      }
      options_->save();
      navigateTo(parentFactory_);
     });
}
void OptionsScreen::render(int mouseX, int mouseY, float tickDelta) {
 renderBackground();
 if(textRenderer() != nullptr) {
  drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 20, 0xFFFFFF);
 }
 Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::gui::screen::option
