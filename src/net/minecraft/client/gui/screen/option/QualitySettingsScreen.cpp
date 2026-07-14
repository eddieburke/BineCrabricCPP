#include "net/minecraft/client/gui/screen/option/QualitySettingsScreen.hpp"
#include <array>
#include <cstdio>
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace quality_screen {
namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;
std::array<OptionSpec, 9> kSpecs{{
    d::makeCycle("fancyGraphics",
                 11,
                 ApplyFlags::ReloadWorld,
                 d::cycleBoolMember<&GameOptions::fancyGraphics>,
                 d::loadBoolMember<&GameOptions::fancyGraphics>,
                 d::saveBoolMember<&GameOptions::fancyGraphics>,
                 d::getBoolMember<&GameOptions::fancyGraphics>),
    d::makeCycle("viewDistance",
                 4,
                 ApplyFlags::ApplyToWorld,
                 d::cycleIntMod<&GameOptions::viewDistance, 4>,
                 d::loadIntMember<&GameOptions::viewDistance>,
                 d::saveIntMember<&GameOptions::viewDistance>),
    d::makeSlider("renderScale",
                  16,
                  ApplyFlags::None,
                  ApplyFlags::ApplyToWorld | ApplyFlags::ReloadWorld,
                  d::getRenderScaleSlider,
                  d::setRenderScaleSlider,
                  d::loadFloatMember<&GameOptions::renderScale>,
                  d::saveFloatMember<&GameOptions::renderScale>),
    d::makeCycle("mipmapLevel",
                 17,
                 ApplyFlags::ApplyDerived | ApplyFlags::ReloadTextures,
                 d::cycleIntMod<&GameOptions::mipmapLevel, 5>,
                 d::loadIntMember<&GameOptions::mipmapLevel>,
                 d::saveIntMember<&GameOptions::mipmapLevel>),
    d::makeToggle("mipmapLinear",
                  18,
                  ApplyFlags::ApplyDerived | ApplyFlags::ReloadTextures,
                  d::getBoolMember<&GameOptions::mipmapLinear>,
                  d::cycleBoolMember<&GameOptions::mipmapLinear>,
                  d::loadBoolMember<&GameOptions::mipmapLinear>,
                  d::saveBoolMember<&GameOptions::mipmapLinear>),
    d::makeHidden("ao",
                  12,
                  ApplyFlags::ReloadWorld,
                  d::getBoolMember<&GameOptions::ao>,
                  d::loadBoolMember<&GameOptions::ao>,
                  d::saveBoolMember<&GameOptions::ao>),
    d::makeSlider("aoLevel",
                  19,
                  ApplyFlags::None,
                  ApplyFlags::ReloadWorld,
                  d::getFloatMember<&GameOptions::aoLevel>,
                  d::setAoLevel,
                  d::loadFloatMember<&GameOptions::aoLevel>,
                  d::saveFloatMember<&GameOptions::aoLevel>),
    d::makeSlider("brightness",
                  15,
                  ApplyFlags::None,
                  ApplyFlags::None,
                  d::getFloatMember<&GameOptions::brightness>,
                  d::setFloatMember<&GameOptions::brightness>,
                  d::loadFloatMember<&GameOptions::brightness>,
                  d::saveFloatMember<&GameOptions::brightness>),
    d::makeToggle("clearWater",
                  20,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::clearWater>,
                  d::cycleBoolMember<&GameOptions::clearWater>,
                  d::loadBoolMember<&GameOptions::clearWater>,
                  d::saveBoolMember<&GameOptions::clearWater>),
}};
class QualitySettingsScreen : public SettingsScreen {
public:
  QualitySettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
      : SettingsScreen(std::move(parentFactory), gameOptions, "Quality Settings") {
  }

protected:
  void buildOptions(OptionGuiBuilder& gui) override {
    const int x1 = gui.gridX(0);
    const int x2 = gui.gridX(1);
    const int y0 = gui.gridY(0);
    constexpr int dy = layout::kRowSpacing;
    const std::string graphics = resource::language::I18n::getTranslation("options.graphics");
    const std::string renderDist = resource::language::I18n::getTranslation("options.renderDistance");
    gui.boolLabels(x1, y0, "fancyGraphics", graphics.c_str(), "Fancy", "Fast");
    gui.i18nCycle(x2,
                  y0,
                  "viewDistance",
                  renderDist.c_str(),
                  {"options.renderDistance.far",
                   "options.renderDistance.normal",
                   "options.renderDistance.short",
                   "options.renderDistance.tiny"},
                  &GameOptions::viewDistance);
    gui.mappedSlider(x1, y0 + dy, "renderScale", d::getRenderScaleSlider, [](const GameOptions& o) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "Render Scale: %.0fx", o.renderScale);
      return std::string(buf);
    });
    gui.customCycle(x2, y0 + dy, "mipmapLevel", [](const GameOptions& o) {
      return optionLabel("Mipmap Level",
                         o.mipmapLevel == 0 ? resource::language::I18n::getTranslation("options.off")
                                            : std::to_string(o.mipmapLevel));
    });
    gui.toggle(x1, y0 + dy * 2, "mipmapLinear", "Mipmap Type");
    gui.slider(x2, y0 + dy * 2, "aoLevel", "Smooth Lighting", [](const GameOptions& o) {
      return o.aoLevel == 0.0f
                 ? optionLabel("Smooth Lighting", resource::language::I18n::getTranslation("options.off"))
                 : optionLabel("Smooth Lighting", std::to_string(static_cast<int>(o.aoLevel * 100.0f)) + "%");
    });
    gui.slider(x1, y0 + dy * 3, "brightness", "Brightness", [](const GameOptions& o) {
      return percentLabel("Brightness", o.brightness);
    });
    gui.toggle(x2, y0 + dy * 3, "clearWater", "Clear Water");
  }
};
} // namespace quality_screen
std::unique_ptr<screen::Screen> makeQualitySettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory, client_option::GameOptions* gameOptions) {
  return std::make_unique<quality_screen::QualitySettingsScreen>(std::move(parentFactory), gameOptions);
}
} // namespace net::minecraft::client::gui::screen::option
