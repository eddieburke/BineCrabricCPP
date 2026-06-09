#include "net/minecraft/client/gui/screen/option/AnimationSettingsScreen.hpp"

#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"

#include <array>

namespace net::minecraft::client::gui::screen::option {
namespace animation_screen {

namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;

std::array<OptionSpec, 8> kSpecs {{
    d::makeCycle("ofAnimatedWater", 36, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::ofAnimatedWater, 2>,
        d::loadIntMember<&GameOptions::ofAnimatedWater>,
        d::saveIntMember<&GameOptions::ofAnimatedWater>),
    d::makeCycle("ofAnimatedLava", 37, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::ofAnimatedLava, 2>,
        d::loadIntMember<&GameOptions::ofAnimatedLava>,
        d::saveIntMember<&GameOptions::ofAnimatedLava>),
    d::makeToggle("ofAnimatedFire", 38, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofAnimatedFire>,
        d::cycleBoolMember<&GameOptions::ofAnimatedFire>,
        d::loadBoolMember<&GameOptions::ofAnimatedFire>,
        d::saveBoolMember<&GameOptions::ofAnimatedFire>),
    d::makeToggle("ofAnimatedPortal", 39, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofAnimatedPortal>,
        d::cycleBoolMember<&GameOptions::ofAnimatedPortal>,
        d::loadBoolMember<&GameOptions::ofAnimatedPortal>,
        d::saveBoolMember<&GameOptions::ofAnimatedPortal>),
    d::makeToggle("ofAnimatedRedstone", 40, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofAnimatedRedstone>,
        d::cycleBoolMember<&GameOptions::ofAnimatedRedstone>,
        d::loadBoolMember<&GameOptions::ofAnimatedRedstone>,
        d::saveBoolMember<&GameOptions::ofAnimatedRedstone>),
    d::makeToggle("ofAnimatedExplosion", 41, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofAnimatedExplosion>,
        d::cycleBoolMember<&GameOptions::ofAnimatedExplosion>,
        d::loadBoolMember<&GameOptions::ofAnimatedExplosion>,
        d::saveBoolMember<&GameOptions::ofAnimatedExplosion>),
    d::makeToggle("ofAnimatedFlame", 42, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofAnimatedFlame>,
        d::cycleBoolMember<&GameOptions::ofAnimatedFlame>,
        d::loadBoolMember<&GameOptions::ofAnimatedFlame>,
        d::saveBoolMember<&GameOptions::ofAnimatedFlame>),
    d::makeToggle("ofAnimatedSmoke", 43, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofAnimatedSmoke>,
        d::cycleBoolMember<&GameOptions::ofAnimatedSmoke>,
        d::loadBoolMember<&GameOptions::ofAnimatedSmoke>,
        d::saveBoolMember<&GameOptions::ofAnimatedSmoke>),
}};

class AnimationSettingsScreen : public SettingsScreen {
public:
    AnimationSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
        : SettingsScreen(std::move(parentFactory), gameOptions, "Animation Settings")
    {
    }

protected:
    void buildOptions(OptionGuiBuilder& gui) override
    {
        const int x1 = gui.gridX(0);
        const int x2 = gui.gridX(1);
        const int y0 = gui.gridY(0);
        constexpr int dy = layout::kRowSpacing;
        const auto animLabel = [](const char* title, int value) {
            return optionLabel(title,
                resource::language::I18n::getTranslation(value == 0 ? "options.on" : "options.off"));
        };

        gui.customCycle(x1, y0, "ofAnimatedWater",
            [animLabel](const GameOptions& o) { return animLabel("Water Animated", o.ofAnimatedWater); });
        gui.customCycle(x2, y0, "ofAnimatedLava",
            [animLabel](const GameOptions& o) { return animLabel("Lava Animated", o.ofAnimatedLava); });
        gui.toggle(x1, y0 + dy, "ofAnimatedFire", "Fire Animated");
        gui.toggle(x2, y0 + dy, "ofAnimatedPortal", "Portal Animated");
        gui.toggle(x1, y0 + dy * 2, "ofAnimatedRedstone", "Redstone Animated");
        gui.toggle(x2, y0 + dy * 2, "ofAnimatedExplosion", "Explosion Animated");
        gui.toggle(x1, y0 + dy * 3, "ofAnimatedFlame", "Flame Animated");
        gui.toggle(x2, y0 + dy * 3, "ofAnimatedSmoke", "Smoke Animated");
    }
};

} // namespace animation_screen

std::unique_ptr<screen::Screen> makeAnimationSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory,
    client_option::GameOptions* gameOptions)
{
    return std::make_unique<animation_screen::AnimationSettingsScreen>(std::move(parentFactory), gameOptions);
}

} // namespace
