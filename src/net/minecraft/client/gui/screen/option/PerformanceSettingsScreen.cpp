#include "net/minecraft/client/gui/screen/option/PerformanceSettingsScreen.hpp"

#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"

#include <array>

namespace net::minecraft::client::gui::screen::option {
namespace performance_screen {

namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;

bool advancedOpenglBool(const GameOptions& o) { return o.advancedOpengl > 0; }

constexpr std::array<int, 5> kPreloadedChunks {0, 2, 4, 6, 8};

void cyclePreloadedChunks(GameOptions& o, int delta)
{
    o.ofPreloadedChunks = d::cycleDiscrete(o.ofPreloadedChunks, delta, kPreloadedChunks.data(),
        static_cast<int>(kPreloadedChunks.size()));
}

std::array<OptionSpec, 10> kSpecs {{
    d::makeCycle("fpsLimit", 9, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::fpsLimit, 3>,
        d::loadIntMember<&GameOptions::fpsLimit>,
        d::saveIntMember<&GameOptions::fpsLimit>),
    d::makeToggle("ofSmoothFps", 21, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofSmoothFps>,
        d::cycleBoolMember<&GameOptions::ofSmoothFps>,
        d::loadBoolMember<&GameOptions::ofSmoothFps>,
        d::saveBoolMember<&GameOptions::ofSmoothFps>),
    d::makeToggle("ofSmoothInput", 22, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofSmoothInput>,
        d::cycleBoolMember<&GameOptions::ofSmoothInput>,
        d::loadBoolMember<&GameOptions::ofSmoothInput>,
        d::saveBoolMember<&GameOptions::ofSmoothInput>),
    d::makeCycle("advancedOpengl", 8, ApplyFlags::ReloadWorld,
        d::cycleIntMod<&GameOptions::advancedOpengl, 3>,
        d::loadIntMember<&GameOptions::advancedOpengl>,
        d::saveIntMember<&GameOptions::advancedOpengl>,
        advancedOpenglBool),
    d::makeToggle("ofVBO", 23, ApplyFlags::ReloadWorld,
        d::getBoolMember<&GameOptions::ofVBO>,
        d::cycleBoolMember<&GameOptions::ofVBO>,
        d::loadBoolMember<&GameOptions::ofVBO>,
        d::saveBoolMember<&GameOptions::ofVBO>),
    d::makeSlider("ofChunkUpdates", 24, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofChunkUpdates>,
        d::setFloatMember<&GameOptions::ofChunkUpdates>,
        d::loadFloatMember<&GameOptions::ofChunkUpdates>,
        d::saveFloatMember<&GameOptions::ofChunkUpdates>),
    d::makeToggle("ofChunkUpdatesDynamic", 25, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofChunkUpdatesDynamic>,
        d::cycleBoolMember<&GameOptions::ofChunkUpdatesDynamic>,
        d::loadBoolMember<&GameOptions::ofChunkUpdatesDynamic>,
        d::saveBoolMember<&GameOptions::ofChunkUpdatesDynamic>),
    d::makeCycle("ofPreloadedChunks", 26, ApplyFlags::ApplyToWorld,
        cyclePreloadedChunks,
        d::loadIntMember<&GameOptions::ofPreloadedChunks>,
        d::saveIntMember<&GameOptions::ofPreloadedChunks>),
    d::makeSlider("ofEntityDistanceScale", 27, ApplyFlags::None, ApplyFlags::None,
        d::getEntityDistanceSlider, d::setEntityDistanceSlider,
        d::loadFloatMember<&GameOptions::ofEntityDistanceScale>,
        d::saveFloatMember<&GameOptions::ofEntityDistanceScale>),
    d::makeToggle("frustumCulling", 13, ApplyFlags::None,
        d::getBoolMember<&GameOptions::frustumCulling>,
        d::cycleBoolMember<&GameOptions::frustumCulling>,
        d::loadBoolMember<&GameOptions::frustumCulling>,
        d::saveBoolMember<&GameOptions::frustumCulling>),
}};

class PerformanceSettingsScreen : public SettingsScreen {
public:
    PerformanceSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
        : SettingsScreen(std::move(parentFactory), gameOptions, "Performance Settings")
    {
    }

protected:
    void buildOptions(OptionGuiBuilder& gui) override
    {
        const int x1 = gui.gridX(0);
        const int x2 = gui.gridX(1);
        const int y0 = gui.gridY(0);
        constexpr int dy = layout::kRowSpacing;
        const std::string fps = resource::language::I18n::getTranslation("options.framerateLimit");
        const std::string advGl = resource::language::I18n::getTranslation("options.advancedOpengl");

        gui.i18nCycle(x1, y0, "fpsLimit", fps.c_str(),
            {"performance.max", "performance.balanced", "performance.powersaver"},
            &GameOptions::fpsLimit);
        gui.toggle(x2, y0, "ofSmoothFps", "Smooth FPS");
        gui.toggle(x1, y0 + dy, "ofSmoothInput", "Smooth Input");
        gui.intCycle(x2, y0 + dy, "advancedOpengl", advGl.c_str(),
            {"OFF", "Fast", "Fancy"}, &GameOptions::advancedOpengl);
        gui.toggle(x1, y0 + dy * 2, "ofVBO", "VBO");
        gui.slider(x2, y0 + dy * 2, "ofChunkUpdates", "Chunk Updates",
            [](const GameOptions& o) { return percentLabel("Chunk Updates", o.ofChunkUpdates); });
        gui.toggle(x1, y0 + dy * 3, "ofChunkUpdatesDynamic", "Dynamic Updates");
        gui.customCycle(x2, y0 + dy * 3, "ofPreloadedChunks",
            [](const GameOptions& o) {
                return optionLabel("Preloaded Chunks",
                    o.ofPreloadedChunks == 0
                        ? resource::language::I18n::getTranslation("options.off")
                        : std::to_string(o.ofPreloadedChunks));
            });
        gui.mappedSlider(x1, y0 + dy * 4, "ofEntityDistanceScale",
            d::getEntityDistanceSlider,
            [](const GameOptions& o) {
                const float pct = 25.0f + d::getEntityDistanceSlider(o) * 375.0f;
                return optionLabel("Entity Distance", std::to_string(static_cast<int>(pct)) + "%");
            });
        gui.toggle(x2, y0 + dy * 4, "frustumCulling", "Frustum Culling");
    }
};

} // namespace performance_screen

std::unique_ptr<screen::Screen> makePerformanceSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory,
    client_option::GameOptions* gameOptions)
{
    return std::make_unique<performance_screen::PerformanceSettingsScreen>(std::move(parentFactory), gameOptions);
}

} // namespace
