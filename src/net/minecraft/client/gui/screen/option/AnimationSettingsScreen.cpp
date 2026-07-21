#include "net/minecraft/client/gui/screen/option/AnimationSettingsScreen.hpp"
#include <array>
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace animation_screen {
namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;
std::array<OptionSpec, 8> kSpecs{{
    d::makeCycle("animatedWater",
                 36,
                 ApplyFlags::None,
                 d::cycleIntMod<&GameOptions::animatedWater, 2>,
                 d::loadIntMember<&GameOptions::animatedWater>,
                 d::saveIntMember<&GameOptions::animatedWater>),
    d::makeCycle("animatedLava",
                 37,
                 ApplyFlags::None,
                 d::cycleIntMod<&GameOptions::animatedLava, 2>,
                 d::loadIntMember<&GameOptions::animatedLava>,
                 d::saveIntMember<&GameOptions::animatedLava>),
    d::makeToggle("animatedFire",
                  38,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::animatedFire>,
                  d::cycleBoolMember<&GameOptions::animatedFire>,
                  d::loadBoolMember<&GameOptions::animatedFire>,
                  d::saveBoolMember<&GameOptions::animatedFire>),
    d::makeToggle("animatedPortal",
                  39,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::animatedPortal>,
                  d::cycleBoolMember<&GameOptions::animatedPortal>,
                  d::loadBoolMember<&GameOptions::animatedPortal>,
                  d::saveBoolMember<&GameOptions::animatedPortal>),
    d::makeToggle("animatedRedstone",
                  40,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::animatedRedstone>,
                  d::cycleBoolMember<&GameOptions::animatedRedstone>,
                  d::loadBoolMember<&GameOptions::animatedRedstone>,
                  d::saveBoolMember<&GameOptions::animatedRedstone>),
    d::makeToggle("animatedExplosion",
                  41,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::animatedExplosion>,
                  d::cycleBoolMember<&GameOptions::animatedExplosion>,
                  d::loadBoolMember<&GameOptions::animatedExplosion>,
                  d::saveBoolMember<&GameOptions::animatedExplosion>),
    d::makeToggle("animatedFlame",
                  42,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::animatedFlame>,
                  d::cycleBoolMember<&GameOptions::animatedFlame>,
                  d::loadBoolMember<&GameOptions::animatedFlame>,
                  d::saveBoolMember<&GameOptions::animatedFlame>),
    d::makeToggle("animatedSmoke",
                  43,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::animatedSmoke>,
                  d::cycleBoolMember<&GameOptions::animatedSmoke>,
                  d::loadBoolMember<&GameOptions::animatedSmoke>,
                  d::saveBoolMember<&GameOptions::animatedSmoke>),
}};
class AnimationSettingsScreen : public SettingsScreen {
 public:
 AnimationSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
     : SettingsScreen(std::move(parentFactory), gameOptions, "Animation Settings") {
 }

 protected:
 void buildOptions(OptionGuiBuilder& gui) override {
  const int x1 = gui.gridX(0);
  const int x2 = gui.gridX(1);
  const int y0 = gui.gridY(0);
  constexpr int dy = layout::kRowSpacing;
  const auto animLabel = [](const char* title, int value) {
   return optionLabel(title,
                      resource::language::I18n::getTranslation(value == 0 ? "options.on" : "options.off"));
  };
  gui.customCycle(x1, y0, "animatedWater", [animLabel](const GameOptions& o) {
   return animLabel("Water Animated", o.animatedWater);
  });
  gui.customCycle(x2, y0, "animatedLava", [animLabel](const GameOptions& o) {
   return animLabel("Lava Animated", o.animatedLava);
  });
  gui.toggle(x1, y0 + dy, "animatedFire", "Fire Animated");
  gui.toggle(x2, y0 + dy, "animatedPortal", "Portal Animated");
  gui.toggle(x1, y0 + dy * 2, "animatedRedstone", "Redstone Animated");
  gui.toggle(x2, y0 + dy * 2, "animatedExplosion", "Explosion Animated");
  gui.toggle(x1, y0 + dy * 3, "animatedFlame", "Flame Animated");
  gui.toggle(x2, y0 + dy * 3, "animatedSmoke", "Smoke Animated");
 }
};
} // namespace animation_screen
std::unique_ptr<screen::Screen> makeAnimationSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory, client_option::GameOptions* gameOptions) {
 return std::make_unique<animation_screen::AnimationSettingsScreen>(std::move(parentFactory), gameOptions);
}
} // namespace net::minecraft::client::gui::screen::option
