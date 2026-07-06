#include "net/minecraft/client/gui/screen/option/FogSettingsScreenFactory.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include <array>
namespace net::minecraft::client::gui::screen::option {
namespace fog_screen {
namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;
bool fogCustomEnabled(const GameOptions& o) {
  return o.fogFancy;
}
bool fogLinearStartEnd(const GameOptions& o) {
  return o.fogFancy && o.fogMode == 0;
}
bool fogExpDensity(const GameOptions& o) {
  return o.fogFancy && o.fogMode != 0;
}
bool fogCustomColor(const GameOptions& o) {
  return o.fogFancy && o.fogColorMode == 1;
}
std::array<OptionSpec, 10> kSpecs{{
    d::makeToggle("fogFancy", 54, ApplyFlags::None, d::getBoolMember<&GameOptions::fogFancy>,
                  d::cycleBoolMember<&GameOptions::fogFancy>, d::loadBoolMember<&GameOptions::fogFancy>,
                  d::saveBoolMember<&GameOptions::fogFancy>),
    d::makeCycle("fogProjection", 55, ApplyFlags::None, d::cycleIntMod<&GameOptions::fogProjection, 2>,
                 d::loadIntMember<&GameOptions::fogProjection>, d::saveIntMember<&GameOptions::fogProjection>,
                 nullptr, fogCustomEnabled),
    d::makeSlider("fogStart", 56, ApplyFlags::None, ApplyFlags::None, d::getFloatMember<&GameOptions::fogStart>,
                  d::setFloatMember<&GameOptions::fogStart>, d::loadFloatMember<&GameOptions::fogStart>,
                  d::saveFloatMember<&GameOptions::fogStart>, fogLinearStartEnd),
    d::makeCycle("fogMode", 57, ApplyFlags::None, d::cycleIntMod<&GameOptions::fogMode, 2>,
                 d::loadIntMember<&GameOptions::fogMode>, d::saveIntMember<&GameOptions::fogMode>, nullptr,
                 fogCustomEnabled),
    d::makeSlider("fogEnd", 58, ApplyFlags::None, ApplyFlags::None, d::getFloatMember<&GameOptions::fogEnd>,
                  d::setFloatMember<&GameOptions::fogEnd>, d::loadFloatMember<&GameOptions::fogEnd>,
                  d::saveFloatMember<&GameOptions::fogEnd>, fogLinearStartEnd),
    d::makeSlider("fogDensity", 59, ApplyFlags::None, ApplyFlags::None, d::getFloatMember<&GameOptions::fogDensity>,
                  d::setFloatMember<&GameOptions::fogDensity>, d::loadFloatMember<&GameOptions::fogDensity>,
                  d::saveFloatMember<&GameOptions::fogDensity>, fogExpDensity),
    d::makeCycle("fogColorMode", 63, ApplyFlags::None, d::cycleIntMod<&GameOptions::fogColorMode, 2>,
                 d::loadIntMember<&GameOptions::fogColorMode>, d::saveIntMember<&GameOptions::fogColorMode>,
                 nullptr, fogCustomEnabled),
    d::makeSlider("fogColorRed", 60, ApplyFlags::None, ApplyFlags::None,
                  d::getFloatMember<&GameOptions::fogColorRed>, d::setFloatMember<&GameOptions::fogColorRed>,
                  d::loadFloatMember<&GameOptions::fogColorRed>, d::saveFloatMember<&GameOptions::fogColorRed>,
                  fogCustomColor),
    d::makeSlider("fogColorGreen", 61, ApplyFlags::None, ApplyFlags::None,
                  d::getFloatMember<&GameOptions::fogColorGreen>, d::setFloatMember<&GameOptions::fogColorGreen>,
                  d::loadFloatMember<&GameOptions::fogColorGreen>, d::saveFloatMember<&GameOptions::fogColorGreen>,
                  fogCustomColor),
    d::makeSlider("fogColorBlue", 62, ApplyFlags::None, ApplyFlags::None,
                  d::getFloatMember<&GameOptions::fogColorBlue>, d::setFloatMember<&GameOptions::fogColorBlue>,
                  d::loadFloatMember<&GameOptions::fogColorBlue>, d::saveFloatMember<&GameOptions::fogColorBlue>,
                  fogCustomColor),
}};
class FogSettingsScreen : public SettingsScreen {
public:
  FogSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
      : SettingsScreen(std::move(parentFactory), gameOptions, "Fog Settings") {
  }

protected:
  void buildOptions(OptionGuiBuilder& gui) override {
    const int x1 = gui.gridX(0);
    const int x2 = gui.gridX(1);
    const int y0 = gui.gridY(0);
    constexpr int dy = layout::kRowSpacing;
    gui.toggle(x1, y0, "fogFancy", "Custom Fog");
    gui.intCycle(x2, y0, "fogProjection", "Fog Projection", {"Spherical", "Cylindrical"},
                 &GameOptions::fogProjection, fogCustomEnabled);
    gui.slider(
        x1, y0 + dy, "fogStart", "Fog Start",
        [](const GameOptions& o) { return percentLabel("Fog Start", o.fogStart); }, fogLinearStartEnd);
    gui.intCycle(x2, y0 + dy, "fogMode", "Fog Mode", {"Linear", "Exp"}, &GameOptions::fogMode,
                 fogCustomEnabled);
    gui.slider(
        x1, y0 + dy * 2, "fogEnd", "Fog End",
        [](const GameOptions& o) { return percentLabel("Fog End", o.fogEnd); }, fogLinearStartEnd);
    gui.slider(
        x2, y0 + dy * 2, "fogDensity", "Fog Density",
        [](const GameOptions& o) { return percentLabel("Fog Density", o.fogDensity); }, fogExpDensity);
    gui.intCycle(x1, y0 + dy * 3, "fogColorMode", "Fog Color Mode", {"Sky Color", "Custom"},
                 &GameOptions::fogColorMode, fogCustomEnabled);
    gui.slider(
        x2, y0 + dy * 3, "fogColorRed", "Fog Red",
        [](const GameOptions& o) { return percentLabel("Fog Red", o.fogColorRed); }, fogCustomColor);
    gui.slider(
        x1, y0 + dy * 4, "fogColorGreen", "Fog Green",
        [](const GameOptions& o) { return percentLabel("Fog Green", o.fogColorGreen); }, fogCustomColor);
    gui.slider(
        x2, y0 + dy * 4, "fogColorBlue", "Fog Blue",
        [](const GameOptions& o) { return percentLabel("Fog Blue", o.fogColorBlue); }, fogCustomColor);
  }
  int doneButtonY() const override {
    return height() / 6 + 24 * 10;
  }
};
} // namespace fog_screen
std::unique_ptr<screen::Screen> makeFogSettingsScreen(std::function<std::unique_ptr<screen::Screen>()> parentFactory,
                                                      client_option::GameOptions* gameOptions) {
  return std::make_unique<fog_screen::FogSettingsScreen>(std::move(parentFactory), gameOptions);
}
} // namespace net::minecraft::client::gui::screen::option
