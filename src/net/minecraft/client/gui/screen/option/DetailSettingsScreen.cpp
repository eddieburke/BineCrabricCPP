#include "net/minecraft/client/gui/screen/option/DetailSettingsScreen.hpp"

#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"

#include <array>

namespace net::minecraft::client::gui::screen::option {
namespace detail_screen {

namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;

std::array<OptionSpec, 8> kSpecs {{
    d::makeCycle("ofClouds", 28, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::ofClouds, 4>,
        d::loadIntMember<&GameOptions::ofClouds>,
        d::saveIntMember<&GameOptions::ofClouds>),
    d::makeSlider("ofCloudsHeight", 29, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofCloudsHeight>,
        d::setFloatMember<&GameOptions::ofCloudsHeight>,
        d::loadFloatMember<&GameOptions::ofCloudsHeight>,
        d::saveFloatMember<&GameOptions::ofCloudsHeight>),
    d::makeCycle("ofTrees", 30, ApplyFlags::ReloadWorld,
        d::cycleIntMod<&GameOptions::ofTrees, 2>,
        d::loadIntMember<&GameOptions::ofTrees>,
        d::saveIntMember<&GameOptions::ofTrees>),
    d::makeCycle("ofGrass", 31, ApplyFlags::ReloadWorld,
        d::cycleIntMod<&GameOptions::ofGrass, 4>,
        d::loadIntMember<&GameOptions::ofGrass>,
        d::saveIntMember<&GameOptions::ofGrass>),
    d::makeCycle("ofWater", 32, ApplyFlags::ReloadWorld,
        d::cycleIntMod<&GameOptions::ofWater, 3>,
        d::loadIntMember<&GameOptions::ofWater>,
        d::saveIntMember<&GameOptions::ofWater>),
    d::makeCycle("ofRain", 33, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::ofRain, 3>,
        d::loadIntMember<&GameOptions::ofRain>,
        d::saveIntMember<&GameOptions::ofRain>),
    d::makeToggle("ofSky", 34, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofSky>,
        d::cycleBoolMember<&GameOptions::ofSky>,
        d::loadBoolMember<&GameOptions::ofSky>,
        d::saveBoolMember<&GameOptions::ofSky>),
    d::makeToggle("ofStars", 35, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofStars>,
        d::cycleBoolMember<&GameOptions::ofStars>,
        d::loadBoolMember<&GameOptions::ofStars>,
        d::saveBoolMember<&GameOptions::ofStars>),
}};

class DetailSettingsScreen : public SettingsScreen {
public:
    DetailSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
        : SettingsScreen(std::move(parentFactory), gameOptions, "Detail Settings")
    {
    }

protected:
    void buildOptions(OptionGuiBuilder& gui) override
    {
        const int x1 = gui.gridX(0);
        const int x2 = gui.gridX(1);
        const int y0 = gui.gridY(0);
        constexpr int dy = layout::kRowSpacing;

        gui.intCycle(x1, y0, "ofClouds", "Clouds", {"Fancy", "Fast", "OFF", "OFF"}, &GameOptions::ofClouds);
        gui.slider(x2, y0, "ofCloudsHeight", "Cloud Height",
            [](const GameOptions& o) {
                return o.ofCloudsHeight == 0.0f
                    ? optionLabel("Cloud Height", "OFF")
                    : optionLabel("Cloud Height", std::to_string(static_cast<int>(o.ofCloudsHeight * 100.0f)) + "%");
            });
        gui.intCycle(x1, y0 + dy, "ofTrees", "Trees", {"Fancy", "Fast"}, &GameOptions::ofTrees);
        gui.intCycle(x2, y0 + dy, "ofGrass", "Grass",
            {"Fancy", "Fast", "Better Fancy", "Better Fast"}, &GameOptions::ofGrass);
        gui.intCycle(x1, y0 + dy * 2, "ofWater", "Water", {"Fancy", "Fast", "OFF"}, &GameOptions::ofWater);
        gui.intCycle(x2, y0 + dy * 2, "ofRain", "Rain & Snow", {"Fancy", "Fast", "OFF"}, &GameOptions::ofRain);
        gui.toggle(x1, y0 + dy * 3, "ofSky", "Sky");
        gui.toggle(x2, y0 + dy * 3, "ofStars", "Stars");
    }
};

} // namespace detail_screen

std::unique_ptr<screen::Screen> makeDetailSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory,
    client_option::GameOptions* gameOptions)
{
    return std::make_unique<detail_screen::DetailSettingsScreen>(std::move(parentFactory), gameOptions);
}

} // namespace
