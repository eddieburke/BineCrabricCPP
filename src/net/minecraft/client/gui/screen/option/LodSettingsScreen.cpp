#include "net/minecraft/client/gui/screen/option/LodSettingsScreen.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace lod_screen {
namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;
bool lodControlsEnabled(const GameOptions& o) {
 return o.distantTerrain &&
        (o.minecraft == nullptr || o.minecraft->gameRenderer == nullptr ||
         net::minecraft::client::option::supportsDistantTerrain(o.minecraft->gameRenderer->packDefinition()));
}
bool distantDetailEnabled(const GameOptions& o) {
 return lodControlsEnabled(o) && net::minecraft::client::option::renderSettings(o).distantTerrain;
}
void cycleNativeRadius(GameOptions& o, int delta) {
 constexpr int values[] = {8, 12, 16, 24, 32};
 o.lodNativeRadius = d::cycleDiscrete(o.lodNativeRadius, delta, values, 5);
}
void cycleMemory(GameOptions& o, int delta) {
 constexpr int values[] = {64, 128, 256, 512};
 o.lodMemoryMiB = d::cycleDiscrete(o.lodMemoryMiB, delta, values, 4);
}
std::array<OptionSpec, 4> kSpecs{{
 d::makeToggle("distantTerrain", 72, ApplyFlags::None,
               d::getBoolMember<&GameOptions::distantTerrain>, d::cycleBoolMember<&GameOptions::distantTerrain>,
               d::loadBoolMember<&GameOptions::distantTerrain>, d::saveBoolMember<&GameOptions::distantTerrain>),
 d::makeCycle("lodNativeRadius", 73, ApplyFlags::None, cycleNativeRadius,
              d::loadIntMember<&GameOptions::lodNativeRadius>, d::saveIntMember<&GameOptions::lodNativeRadius>, nullptr, lodControlsEnabled),
 d::makeCycle("lodQuality", 74, ApplyFlags::None, d::cycleIntMod<&GameOptions::lodQuality, 3>,
              d::loadIntMember<&GameOptions::lodQuality>, d::saveIntMember<&GameOptions::lodQuality>, nullptr, distantDetailEnabled),
 d::makeCycle("lodMemoryMiB", 75, ApplyFlags::None, cycleMemory,
              d::loadIntMember<&GameOptions::lodMemoryMiB>, d::saveIntMember<&GameOptions::lodMemoryMiB>, nullptr, distantDetailEnabled)
}};
class LodSettingsScreen : public SettingsScreen {
 public:
 LodSettingsScreen(ParentFactory parentFactory, GameOptions* options)
     : SettingsScreen(std::move(parentFactory), options, "Terrain LOD Settings") {}
 protected:
 void buildOptions(OptionGuiBuilder& gui) override {
  const int x1 = gui.gridX(0);
  const int x2 = gui.gridX(1);
  gui.customCycle(x1, gui.gridY(0), "distantTerrain", [](const GameOptions& o) {
   if(!o.distantTerrain) return optionLabel("Terrain LOD", "Off");
   if(!lodControlsEnabled(o)) return std::string("LOD: Shader unsupported");
   return distantDetailEnabled(o) ? optionLabel("Terrain LOD", "On") : std::string("LOD: No distant range");
  });
  gui.customCycle(x1, gui.gridY(1), "lodNativeRadius", [](const GameOptions& o) {
   return optionLabel("Full Detail", std::to_string(o.lodNativeRadius) + " chunks");
  });
  gui.customCycle(x2, gui.gridY(1), "lodQuality", [](const GameOptions& o) {
   constexpr const char* labels[] = {"High", "Balanced", "Fast"};
   return optionLabel("Distant Detail", labels[std::clamp(o.lodQuality, 0, 2)]);
  });
  gui.customCycle(x1, gui.gridY(2), "lodMemoryMiB", [](const GameOptions& o) {
   return optionLabel("Distant Memory", std::to_string(o.lodMemoryMiB) + " MiB");
  });
 }
};
}
std::unique_ptr<screen::Screen> makeLodSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory,
    net::minecraft::client::option::GameOptions* gameOptions) {
 return std::make_unique<lod_screen::LodSettingsScreen>(std::move(parentFactory), gameOptions);
}
}
