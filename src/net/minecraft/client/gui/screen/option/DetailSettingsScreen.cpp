#include "net/minecraft/client/gui/screen/option/DetailSettingsScreen.hpp"

#include <array>

#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"

namespace net::minecraft::client::gui::screen::option {
namespace detail_screen {
namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;

[[nodiscard]] bool cloudsHeightEnabled(const GameOptions& options) noexcept {
    return (options.clouds & 3) != 2;
}

std::array<OptionSpec, 8> kSpecs{{
    d::makeCycle("clouds",
                 28,
                 ApplyFlags::None,
                 d::cycleIntMod<&GameOptions::clouds, 4>,
                 d::loadIntMember<&GameOptions::clouds>,
                 d::saveIntMember<&GameOptions::clouds>),
    d::makeSlider("cloudsHeight",
                  29,
                  ApplyFlags::None,
                  ApplyFlags::None,
                  d::getFloatMember<&GameOptions::cloudsHeight>,
                  d::setFloatMember<&GameOptions::cloudsHeight>,
                  d::loadFloatMember<&GameOptions::cloudsHeight>,
                  d::saveFloatMember<&GameOptions::cloudsHeight>),
    d::makeCycle("trees",
                 30,
                 ApplyFlags::ReloadWorld,
                 d::cycleIntMod<&GameOptions::trees, 2>,
                 d::loadIntMember<&GameOptions::trees>,
                 d::saveIntMember<&GameOptions::trees>),
    d::makeCycle("grass",
                 31,
                 ApplyFlags::ReloadWorld,
                 d::cycleIntMod<&GameOptions::grass, 4>,
                 d::loadIntMember<&GameOptions::grass>,
                 d::saveIntMember<&GameOptions::grass>),
    d::makeCycle("water",
                 32,
                 ApplyFlags::ReloadWorld,
                 d::cycleIntMod<&GameOptions::water, 3>,
                 d::loadIntMember<&GameOptions::water>,
                 d::saveIntMember<&GameOptions::water>),
    d::makeCycle("rain",
                 33,
                 ApplyFlags::None,
                 d::cycleIntMod<&GameOptions::rain, 3>,
                 d::loadIntMember<&GameOptions::rain>,
                 d::saveIntMember<&GameOptions::rain>),
    d::makeToggle("sky",
                  34,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::sky>,
                  d::cycleBoolMember<&GameOptions::sky>,
                  d::loadBoolMember<&GameOptions::sky>,
                  d::saveBoolMember<&GameOptions::sky>),
    d::makeToggle("stars",
                  35,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::stars>,
                  d::cycleBoolMember<&GameOptions::stars>,
                  d::loadBoolMember<&GameOptions::stars>,
                  d::saveBoolMember<&GameOptions::stars>),
}};

class DetailSettingsScreen : public SettingsScreen {
   public:
    DetailSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
        : SettingsScreen(std::move(parentFactory), gameOptions, "Detail Settings") {
    }

    [[nodiscard]] std::string_view getScreenUiId() const override {
        return mod::screen_ids::kDetailSettings;
    }

   protected:
    void buildOptions(OptionGuiBuilder& gui) override {
        const int x1 = gui.gridX(0);
        const int x2 = gui.gridX(1);
        const int y0 = gui.gridY(0);
        constexpr int dy = layout::kRowSpacing;
        gui.intCycle(x1, y0, "clouds", "Clouds", {"Fancy", "Fast", "OFF"}, &GameOptions::clouds);
        gui.slider(
            x2,
            y0,
            "cloudsHeight",
            "Cloud Height",
            [](const GameOptions& o) {
                return o.cloudsHeight == 0.0f
                           ? optionLabel("Cloud Height", "OFF")
                           : optionLabel("Cloud Height",
                                         std::to_string(static_cast<int>(o.cloudsHeight * 100.0f)) + "%");
            },
            cloudsHeightEnabled);
        gui.intCycle(x1, y0 + dy, "trees", "Trees", {"Fancy", "Fast"}, &GameOptions::trees);
        gui.intCycle(
            x2, y0 + dy, "grass", "Grass", {"Fancy", "Fast", "Better Fancy", "Better Fast"}, &GameOptions::grass);
        gui.intCycle(x1, y0 + dy * 2, "water", "Water", {"Fancy", "Fast", "OFF"}, &GameOptions::water);
        gui.intCycle(x2, y0 + dy * 2, "rain", "Rain & Snow", {"Fancy", "Fast", "OFF"}, &GameOptions::rain);
        gui.toggle(x1, y0 + dy * 3, "sky", "Sky");
        gui.toggle(x2, y0 + dy * 3, "stars", "Stars");
    }
};
}  // namespace detail_screen

std::unique_ptr<screen::Screen> makeDetailSettingsScreen(std::function<std::unique_ptr<screen::Screen>()> parentFactory,
                                                         client_option::GameOptions* gameOptions) {
    return std::make_unique<detail_screen::DetailSettingsScreen>(std::move(parentFactory), gameOptions);
}
}  // namespace net::minecraft::client::gui::screen::option
