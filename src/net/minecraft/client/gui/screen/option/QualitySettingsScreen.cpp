#include "net/minecraft/client/gui/screen/option/QualitySettingsScreen.hpp"

#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"

#include <array>
#include <cstdio>

namespace net::minecraft::client::gui::screen::option {
namespace quality_screen {

namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;

std::array<OptionSpec, 9> kSpecs {{
    d::makeCycle("fancyGraphics", 11, ApplyFlags::ReloadWorld,
        d::cycleBoolMember<&GameOptions::fancyGraphics>,
        d::loadBoolMember<&GameOptions::fancyGraphics>,
        d::saveBoolMember<&GameOptions::fancyGraphics>,
        d::getBoolMember<&GameOptions::fancyGraphics>),
    d::makeCycle("viewDistance", 4, ApplyFlags::None,
        d::cycleIntMod<&GameOptions::viewDistance, 4>,
        d::loadIntMember<&GameOptions::viewDistance>,
        d::saveIntMember<&GameOptions::viewDistance>),
    d::makeSlider("ofRenderScale", 16, ApplyFlags::None, ApplyFlags::ReloadWorld,
        d::getRenderScaleSlider, d::setRenderScaleSlider,
        d::loadFloatMember<&GameOptions::ofRenderScale>,
        d::saveFloatMember<&GameOptions::ofRenderScale>),
    d::makeCycle("ofMipmapLevel", 17, ApplyFlags::ApplyDerived | ApplyFlags::ReloadTextures,
        d::cycleIntMod<&GameOptions::ofMipmapLevel, 5>,
        d::loadIntMember<&GameOptions::ofMipmapLevel>,
        d::saveIntMember<&GameOptions::ofMipmapLevel>),
    d::makeToggle("ofMipmapLinear", 18, ApplyFlags::ReloadTextures,
        d::getBoolMember<&GameOptions::ofMipmapLinear>,
        d::cycleBoolMember<&GameOptions::ofMipmapLinear>,
        d::loadBoolMember<&GameOptions::ofMipmapLinear>,
        d::saveBoolMember<&GameOptions::ofMipmapLinear>),
    d::makeHidden("ao", 12, ApplyFlags::ReloadWorld,
        d::getBoolMember<&GameOptions::ao>,
        d::loadBoolMember<&GameOptions::ao>,
        d::saveBoolMember<&GameOptions::ao>),
    d::makeSlider("ofAoLevel", 19, ApplyFlags::None, ApplyFlags::ReloadWorld,
        d::getFloatMember<&GameOptions::ofAoLevel>, d::setAoLevel,
        d::loadFloatMember<&GameOptions::ofAoLevel>,
        d::saveFloatMember<&GameOptions::ofAoLevel>),
    d::makeSlider("ofBrightness", 15, ApplyFlags::None, ApplyFlags::None,
        d::getFloatMember<&GameOptions::ofBrightness>,
        d::setFloatMember<&GameOptions::ofBrightness>,
        d::loadFloatMember<&GameOptions::ofBrightness>,
        d::saveFloatMember<&GameOptions::ofBrightness>),
    d::makeToggle("ofClearWater", 20, ApplyFlags::None,
        d::getBoolMember<&GameOptions::ofClearWater>,
        d::cycleBoolMember<&GameOptions::ofClearWater>,
        d::loadBoolMember<&GameOptions::ofClearWater>,
        d::saveBoolMember<&GameOptions::ofClearWater>),
}};

class QualitySettingsScreen : public SettingsScreen {
public:
    QualitySettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
        : SettingsScreen(std::move(parentFactory), gameOptions, "Quality Settings")
    {
    }

protected:
    void buildOptions(OptionGuiBuilder& gui) override
    {
        const int x1 = gui.gridX(0);
        const int x2 = gui.gridX(1);
        const int y0 = gui.gridY(0);
        constexpr int dy = layout::kRowSpacing;
        const std::string graphics = resource::language::I18n::getTranslation("options.graphics");
        const std::string renderDist = resource::language::I18n::getTranslation("options.renderDistance");

        gui.boolLabels(x1, y0, "fancyGraphics", graphics.c_str(), "Fancy", "Fast");
        gui.i18nCycle(x2, y0, "viewDistance", renderDist.c_str(),
            {"options.renderDistance.far", "options.renderDistance.normal",
             "options.renderDistance.short", "options.renderDistance.tiny"},
            &GameOptions::viewDistance);
        gui.mappedSlider(x1, y0 + dy, "ofRenderScale",
            d::getRenderScaleSlider,
            [](const GameOptions& o) {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "Render Scale: %.0fx", o.ofRenderScale);
                return std::string(buf);
            });
        gui.customCycle(x2, y0 + dy, "ofMipmapLevel",
            [](const GameOptions& o) {
                return optionLabel("Mipmap Level",
                    o.ofMipmapLevel == 0
                        ? resource::language::I18n::getTranslation("options.off")
                        : std::to_string(o.ofMipmapLevel));
            });
        gui.toggle(x1, y0 + dy * 2, "ofMipmapLinear", "Mipmap Type");
        gui.slider(x2, y0 + dy * 2, "ofAoLevel", "Smooth Lighting",
            [](const GameOptions& o) {
                return o.ofAoLevel == 0.0f
                    ? optionLabel("Smooth Lighting", resource::language::I18n::getTranslation("options.off"))
                    : optionLabel("Smooth Lighting", std::to_string(static_cast<int>(o.ofAoLevel * 100.0f)) + "%");
            });
        gui.slider(x1, y0 + dy * 3, "ofBrightness", "Brightness",
            [](const GameOptions& o) { return percentLabel("Brightness", o.ofBrightness); });
        gui.toggle(x2, y0 + dy * 3, "ofClearWater", "Clear Water");
    }
};

} // namespace quality_screen

std::unique_ptr<screen::Screen> makeQualitySettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory,
    client_option::GameOptions* gameOptions)
{
    return std::make_unique<quality_screen::QualitySettingsScreen>(std::move(parentFactory), gameOptions);
}

} // namespace
