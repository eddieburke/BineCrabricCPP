#include "net/minecraft/client/gui/screen/option/FarViewSettingsScreen.hpp"
#include <array>
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace far_view_screen {
namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;
std::array<OptionSpec, 5> kSpecs{{
    d::makeToggle("lodEnabled",
                  60,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::lodEnabled>,
                  d::cycleBoolMember<&GameOptions::lodEnabled>,
                  d::loadBoolMember<&GameOptions::lodEnabled>,
                  d::saveBoolMember<&GameOptions::lodEnabled>),
    d::makeCycle("lodDistance",
                 61,
                 ApplyFlags::None,
                 d::cycleIntMod<&GameOptions::lodDistance, 4>,
                 d::loadIntMember<&GameOptions::lodDistance>,
                 d::saveIntMember<&GameOptions::lodDistance>),
    d::makeCycle("lodDetail",
                 62,
                 ApplyFlags::None,
                 d::cycleIntMod<&GameOptions::lodDetail, 3>,
                 d::loadIntMember<&GameOptions::lodDetail>,
                 d::saveIntMember<&GameOptions::lodDetail>),
    d::makeToggle("lodFogExtend",
                  63,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::lodFogExtend>,
                  d::cycleBoolMember<&GameOptions::lodFogExtend>,
                  d::loadBoolMember<&GameOptions::lodFogExtend>,
                  d::saveBoolMember<&GameOptions::lodFogExtend>),
    d::makeToggle("lodImportWorld",
                  64,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::lodImportWorld>,
                  d::cycleBoolMember<&GameOptions::lodImportWorld>,
                  d::loadBoolMember<&GameOptions::lodImportWorld>,
                  d::saveBoolMember<&GameOptions::lodImportWorld>),
}};
class FarViewSettingsScreen : public SettingsScreen {
public:
  FarViewSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
      : SettingsScreen(std::move(parentFactory), gameOptions, "Far View Settings") {
  }

protected:
  void buildOptions(OptionGuiBuilder& gui) override {
    const int x1 = gui.gridX(0);
    const int x2 = gui.gridX(1);
    const int y0 = gui.gridY(0);
    constexpr int dy = layout::kRowSpacing;
    gui.toggle(x1, y0, "lodEnabled", "Far View LOD");
    gui.customCycle(x2, y0, "lodDistance", [](const GameOptions& o) {
      static constexpr std::array<const char*, 4> labels{"1024", "2048", "4096", "8192"};
      const int idx = ((o.lodDistance % 4) + 4) % 4;
      return optionLabel("LOD Distance", labels[static_cast<std::size_t>(idx)]);
    });
    gui.customCycle(x1, y0 + dy, "lodDetail", [](const GameOptions& o) {
      static constexpr std::array<const char*, 3> labels{"Low", "Normal", "High"};
      const int idx = ((o.lodDetail % 3) + 3) % 3;
      return optionLabel("LOD Detail", labels[static_cast<std::size_t>(idx)]);
    });
    gui.toggle(x2, y0 + dy, "lodFogExtend", "Extend Fog");
    gui.toggle(x1, y0 + dy * 2, "lodImportWorld", "Import Whole World");
  }
};
} // namespace far_view_screen
std::unique_ptr<screen::Screen> makeFarViewSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory, client_option::GameOptions* gameOptions) {
  return std::make_unique<far_view_screen::FarViewSettingsScreen>(std::move(parentFactory), gameOptions);
}
} // namespace net::minecraft::client::gui::screen::option
