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

bool fogCustomEnabled(const GameOptions& o) { return o.ofFogFancy; }
bool fogLinearStartEnd(const GameOptions& o) { return o.ofFogFancy && o.ofFogMode == 0; }
bool fogExpDensity(const GameOptions& o) { return o.ofFogFancy && o.ofFogMode != 0; }
bool fogCustomColor(const GameOptions& o) { return o.ofFogFancy && o.ofFogColorMode == 1; }

std::array<OptionSpec, 10> kSpecs {{
    d::makeToggle("ofFogFancy", 54, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofFogFancy>,
        d::cycleBoolMember<&GameOptions::ofFogFancy>,
        d::loadBoolMember<&GameOptions::ofFogFancy>,
        d::saveBoolMember<&GameOptions::ofFogFancy>),
    d::makeCycle("ofFogProjection", 55, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::ofFogProjection, 2>,
        d::loadIntMember<&GameOptions::ofFogProjection>,
        d::saveIntMember<&GameOptions::ofFogProjection>,
        nullptr, fogCustomEnabled),
    d::makeSlider("ofFogStart", 56, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofFogStart>,
        d::setFloatMember<&GameOptions::ofFogStart>,
        d::loadFloatMember<&GameOptions::ofFogStart>,
        d::saveFloatMember<&GameOptions::ofFogStart>,
        fogLinearStartEnd),
    d::makeCycle("ofFogMode", 57, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::ofFogMode, 2>,
        d::loadIntMember<&GameOptions::ofFogMode>,
        d::saveIntMember<&GameOptions::ofFogMode>,
        nullptr, fogCustomEnabled),
    d::makeSlider("ofFogEnd", 58, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofFogEnd>,
        d::setFloatMember<&GameOptions::ofFogEnd>,
        d::loadFloatMember<&GameOptions::ofFogEnd>,
        d::saveFloatMember<&GameOptions::ofFogEnd>,
        fogLinearStartEnd),
    d::makeSlider("ofFogDensity", 59, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofFogDensity>,
        d::setFloatMember<&GameOptions::ofFogDensity>,
        d::loadFloatMember<&GameOptions::ofFogDensity>,
        d::saveFloatMember<&GameOptions::ofFogDensity>,
        fogExpDensity),
    d::makeCycle("ofFogColorMode", 63, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::ofFogColorMode, 2>,
        d::loadIntMember<&GameOptions::ofFogColorMode>,
        d::saveIntMember<&GameOptions::ofFogColorMode>,
        nullptr, fogCustomEnabled),
    d::makeSlider("ofFogColorRed", 60, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofFogColorRed>,
        d::setFloatMember<&GameOptions::ofFogColorRed>,
        d::loadFloatMember<&GameOptions::ofFogColorRed>,
        d::saveFloatMember<&GameOptions::ofFogColorRed>,
        fogCustomColor),
    d::makeSlider("ofFogColorGreen", 61, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofFogColorGreen>,
        d::setFloatMember<&GameOptions::ofFogColorGreen>,
        d::loadFloatMember<&GameOptions::ofFogColorGreen>,
        d::saveFloatMember<&GameOptions::ofFogColorGreen>,
        fogCustomColor),
    d::makeSlider("ofFogColorBlue", 62, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofFogColorBlue>,
        d::setFloatMember<&GameOptions::ofFogColorBlue>,
        d::loadFloatMember<&GameOptions::ofFogColorBlue>,
        d::saveFloatMember<&GameOptions::ofFogColorBlue>,
        fogCustomColor),
}};

class FogSettingsScreen : public SettingsScreen {
public:
    FogSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
        : SettingsScreen(std::move(parentFactory), gameOptions, "Fog Settings")
    {
    }

protected:
    void buildOptions(OptionGuiBuilder& gui) override
    {
        const int x1 = gui.gridX(0);
        const int x2 = gui.gridX(1);
        const int y0 = gui.gridY(0);
        constexpr int dy = layout::kRowSpacing;

        gui.toggle(x1, y0, "ofFogFancy", "Custom Fog");
        gui.intCycle(x2, y0, "ofFogProjection", "Fog Projection",
            {"Spherical", "Cylindrical"}, &GameOptions::ofFogProjection, fogCustomEnabled);
        gui.slider(x1, y0 + dy, "ofFogStart", "Fog Start",
            [](const GameOptions& o) { return percentLabel("Fog Start", o.ofFogStart); },
            fogLinearStartEnd);
        gui.intCycle(x2, y0 + dy, "ofFogMode", "Fog Mode", {"Linear", "Exp"},
            &GameOptions::ofFogMode, fogCustomEnabled);
        gui.slider(x1, y0 + dy * 2, "ofFogEnd", "Fog End",
            [](const GameOptions& o) { return percentLabel("Fog End", o.ofFogEnd); },
            fogLinearStartEnd);
        gui.slider(x2, y0 + dy * 2, "ofFogDensity", "Fog Density",
            [](const GameOptions& o) { return percentLabel("Fog Density", o.ofFogDensity); },
            fogExpDensity);
        gui.intCycle(x1, y0 + dy * 3, "ofFogColorMode", "Fog Color Mode",
            {"Sky Color", "Custom"}, &GameOptions::ofFogColorMode, fogCustomEnabled);
        gui.slider(x2, y0 + dy * 3, "ofFogColorRed", "Fog Red",
            [](const GameOptions& o) { return percentLabel("Fog Red", o.ofFogColorRed); },
            fogCustomColor);
        gui.slider(x1, y0 + dy * 4, "ofFogColorGreen", "Fog Green",
            [](const GameOptions& o) { return percentLabel("Fog Green", o.ofFogColorGreen); },
            fogCustomColor);
        gui.slider(x2, y0 + dy * 4, "ofFogColorBlue", "Fog Blue",
            [](const GameOptions& o) { return percentLabel("Fog Blue", o.ofFogColorBlue); },
            fogCustomColor);
    }

    int doneButtonY() const override { return height() / 6 + 24 * 10; }
};

} // namespace fog_screen

std::unique_ptr<screen::Screen> makeFogSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory,
    client_option::GameOptions* gameOptions)
{
    return std::make_unique<fog_screen::FogSettingsScreen>(std::move(parentFactory), gameOptions);
}

} // namespace
