#include "net/minecraft/client/gui/screen/option/PerformanceSettingsScreen.hpp"

#include <array>

#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"

namespace net::minecraft::client::gui::screen::option {
namespace performance_screen {
namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;
constexpr std::array<int, 5> kPreloadedChunks{0, 2, 4, 6, 8};

void cyclePreloadedChunks(GameOptions& o, int delta) {
    o.preloadedChunks =
        d::cycleDiscrete(o.preloadedChunks, delta, kPreloadedChunks.data(), static_cast<int>(kPreloadedChunks.size()));
}

std::array<OptionSpec, 10> kSpecs{{
    d::makeCycle("fpsLimit",
                 9,
                 ApplyFlags::None,
                 d::cycleIntMod<&GameOptions::fpsLimit, 3>,
                 d::loadIntMember<&GameOptions::fpsLimit>,
                 d::saveIntMember<&GameOptions::fpsLimit>),
    d::makeToggle("smoothFps",
                  21,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::smoothFps>,
                  d::cycleBoolMember<&GameOptions::smoothFps>,
                  d::loadBoolMember<&GameOptions::smoothFps>,
                  d::saveBoolMember<&GameOptions::smoothFps>),
    d::makeToggle("smoothInput",
                  22,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::smoothInput>,
                  d::cycleBoolMember<&GameOptions::smoothInput>,
                  d::loadBoolMember<&GameOptions::smoothInput>,
                  d::saveBoolMember<&GameOptions::smoothInput>),
    d::makeToggle("entityShadows",
                  8,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::entityShadows>,
                  d::cycleBoolMember<&GameOptions::entityShadows>,
                  d::loadBoolMember<&GameOptions::entityShadows>,
                  d::saveBoolMember<&GameOptions::entityShadows>),
    d::makeToggle("vbo",
                  23,
                  ApplyFlags::ReloadWorld,
                  d::getBoolMember<&GameOptions::vbo>,
                  d::cycleBoolMember<&GameOptions::vbo>,
                  d::loadBoolMember<&GameOptions::vbo>,
                  d::saveBoolMember<&GameOptions::vbo>),
    d::makeSlider("chunkUpdates",
                  24,
                  ApplyFlags::None,
                  ApplyFlags::None,
                  d::getFloatMember<&GameOptions::chunkUpdates>,
                  d::setFloatMember<&GameOptions::chunkUpdates>,
                  d::loadFloatMember<&GameOptions::chunkUpdates>,
                  d::saveFloatMember<&GameOptions::chunkUpdates>),
    d::makeToggle("chunkUpdatesDynamic",
                  25,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::chunkUpdatesDynamic>,
                  d::cycleBoolMember<&GameOptions::chunkUpdatesDynamic>,
                  d::loadBoolMember<&GameOptions::chunkUpdatesDynamic>,
                  d::saveBoolMember<&GameOptions::chunkUpdatesDynamic>),
    d::makeCycle("preloadedChunks",
                 26,
                 ApplyFlags::ApplyToWorld,
                 cyclePreloadedChunks,
                 d::loadIntMember<&GameOptions::preloadedChunks>,
                 d::saveIntMember<&GameOptions::preloadedChunks>),
    d::makeSlider("entityDistanceScale",
                  27,
                  ApplyFlags::None,
                  ApplyFlags::None,
                  d::getEntityDistanceSlider,
                  d::setEntityDistanceSlider,
                  d::loadFloatMember<&GameOptions::entityDistanceScale>,
                  d::saveFloatMember<&GameOptions::entityDistanceScale>),
    d::makeToggle("frustumCulling",
                  13,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::frustumCulling>,
                  d::cycleBoolMember<&GameOptions::frustumCulling>,
                  d::loadBoolMember<&GameOptions::frustumCulling>,
                  d::saveBoolMember<&GameOptions::frustumCulling>),
}};

class PerformanceSettingsScreen : public SettingsScreen {
   public:
    PerformanceSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
        : SettingsScreen(std::move(parentFactory), gameOptions, "Performance Settings") {
    }

   protected:
    void buildOptions(OptionGuiBuilder& gui) override {
        const int x1 = gui.gridX(0);
        const int x2 = gui.gridX(1);
        const int y0 = gui.gridY(0);
        constexpr int dy = layout::kRowSpacing;
        const std::string fps = resource::language::I18n::getTranslation("options.framerateLimit");
        gui.i18nCycle(x1,
                      y0,
                      "fpsLimit",
                      fps.c_str(),
                      {"performance.max", "performance.balanced", "performance.powersaver"},
                      &GameOptions::fpsLimit);
        gui.toggle(x2, y0, "smoothFps", "Smooth FPS");
        gui.toggle(x1, y0 + dy, "smoothInput", "Smooth Input");
        gui.toggle(x2, y0 + dy, "entityShadows", "Entity Shadows");
        gui.toggle(x1, y0 + dy * 2, "vbo", "VBO");
        gui.slider(x2, y0 + dy * 2, "chunkUpdates", "Chunk Updates", [](const GameOptions& o) {
            return percentLabel("Chunk Updates", o.chunkUpdates);
        });
        gui.toggle(x1, y0 + dy * 3, "chunkUpdatesDynamic", "Dynamic Updates");
        gui.customCycle(x2, y0 + dy * 3, "preloadedChunks", [](const GameOptions& o) {
            return optionLabel("Preloaded Chunks",
                               o.preloadedChunks == 0 ? resource::language::I18n::getTranslation("options.off")
                                                      : std::to_string(o.preloadedChunks));
        });
        gui.mappedSlider(x1, y0 + dy * 4, "entityDistanceScale", d::getEntityDistanceSlider, [](const GameOptions& o) {
            const float pct = 25.0f + d::getEntityDistanceSlider(o) * 375.0f;
            return optionLabel("Entity Distance", std::to_string(static_cast<int>(pct)) + "%");
        });
        gui.toggle(x2, y0 + dy * 4, "frustumCulling", "Frustum Culling");
    }
};
}  // namespace performance_screen

std::unique_ptr<screen::Screen> makePerformanceSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory, client_option::GameOptions* gameOptions) {
    return std::make_unique<performance_screen::PerformanceSettingsScreen>(std::move(parentFactory), gameOptions);
}
}  // namespace net::minecraft::client::gui::screen::option
