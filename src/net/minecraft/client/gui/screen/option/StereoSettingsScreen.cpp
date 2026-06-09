#include "net/minecraft/client/gui/screen/option/StereoSettingsScreen.hpp"

#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"

#include <array>

namespace net::minecraft::client::gui::screen::option {
namespace stereo_screen {

namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;

std::array<OptionSpec, 7> kSpecs {{
    d::makeCycle("stereoMode", 7, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::stereoMode, 3>,
        d::loadStereoMode,
        d::saveIntMember<&GameOptions::stereoMode>,
        nullptr, nullptr, d::alwaysResize),
    d::makeSlider("ofStereoOffset", 48, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofStereoOffset>,
        d::setFloatMember<&GameOptions::ofStereoOffset>,
        d::loadFloatMember<&GameOptions::ofStereoOffset>,
        d::saveFloatMember<&GameOptions::ofStereoOffset>),
    d::makeSlider("ofStereoSeparation", 49, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofStereoSeparation>,
        d::setFloatMember<&GameOptions::ofStereoSeparation>,
        d::loadFloatMember<&GameOptions::ofStereoSeparation>,
        d::saveFloatMember<&GameOptions::ofStereoSeparation>),
    d::makeToggle("ofStereoRedBlueOrder", 50, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofStereoRedBlueOrder>,
        d::cycleBoolMember<&GameOptions::ofStereoRedBlueOrder>,
        d::loadBoolMember<&GameOptions::ofStereoRedBlueOrder>,
        d::saveBoolMember<&GameOptions::ofStereoRedBlueOrder>),
    d::makeSlider("ofHandStereoOffset", 51, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofHandStereoOffset>,
        d::setFloatMember<&GameOptions::ofHandStereoOffset>,
        d::loadFloatMember<&GameOptions::ofHandStereoOffset>,
        d::saveFloatMember<&GameOptions::ofHandStereoOffset>),
    d::makeSlider("ofHandStereoSeparation", 52, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofHandStereoSeparation>,
        d::setFloatMember<&GameOptions::ofHandStereoSeparation>,
        d::loadFloatMember<&GameOptions::ofHandStereoSeparation>,
        d::saveFloatMember<&GameOptions::ofHandStereoSeparation>),
    d::makeSlider("ofHandDepth", 53, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofHandDepth>,
        d::setFloatMember<&GameOptions::ofHandDepth>,
        d::loadFloatMember<&GameOptions::ofHandDepth>,
        d::saveFloatMember<&GameOptions::ofHandDepth>),
}};

class StereoSettingsScreen : public SettingsScreen {
public:
    StereoSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
        : SettingsScreen(std::move(parentFactory), gameOptions, "3D Settings")
    {
    }

protected:
    void buildOptions(OptionGuiBuilder& gui) override
    {
        const int x1 = gui.gridX(0);
        const int x2 = gui.gridX(1);
        const int y0 = gui.gridY(0);
        constexpr int dy = layout::kRowSpacing;

        gui.intCycle(x1, y0, "stereoMode", "Stereo Mode",
            {"OFF", "Anaglyph", "Side-by-Side"}, &GameOptions::stereoMode);
        gui.slider(x2, y0, "ofStereoOffset", "3D Offset",
            [](const GameOptions& o) { return percentLabel("3D Offset", o.ofStereoOffset); });
        gui.slider(x1, y0 + dy, "ofStereoSeparation", "3D Separation",
            [](const GameOptions& o) { return percentLabel("3D Separation", o.ofStereoSeparation); });
        gui.toggle(x2, y0 + dy, "ofStereoRedBlueOrder", "Swap Left/Right");
        gui.slider(x1, y0 + dy * 2, "ofHandStereoOffset", "Hand Convergence",
            [](const GameOptions& o) { return percentLabel("Hand Convergence", o.ofHandStereoOffset); });
        gui.slider(x2, y0 + dy * 2, "ofHandStereoSeparation", "Hand Separation",
            [](const GameOptions& o) { return percentLabel("Hand Separation", o.ofHandStereoSeparation); });
        gui.slider(x1, y0 + dy * 3, "ofHandDepth", "Hand Depth",
            [](const GameOptions& o) { return percentLabel("Hand Depth", o.ofHandDepth); });
    }
};

} // namespace stereo_screen

std::unique_ptr<screen::Screen> makeStereoSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory,
    client_option::GameOptions* gameOptions)
{
    return std::make_unique<stereo_screen::StereoSettingsScreen>(std::move(parentFactory), gameOptions);
}

} // namespace
