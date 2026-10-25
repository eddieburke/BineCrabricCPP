#include "net/minecraft/client/gui/screen/option/WorldSettingsScreen.hpp"
#include <array>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::gui::screen::option {
using client_option::GameOptions;
namespace world_screen {
namespace d = net::minecraft::client::option::option_spec_detail;
using net::minecraft::client::option::ApplyFlags;
using net::minecraft::client::option::GameOptions;
using net::minecraft::client::option::OptionSpec;
constexpr std::array<int, 4> kAutoSaveTicks{40, 400, 4000, 40000};
void cycleAutoSaveTicks(GameOptions& o, int delta) {
 o.autoSaveTicks =
     d::cycleDiscrete(o.autoSaveTicks, delta, kAutoSaveTicks.data(), static_cast<int>(kAutoSaveTicks.size()));
}
// A sky mod that claims the celestial phase owns the clock; Time would silently lose
// the fight, so it defers instead and says who has it.
const net::minecraft::World* claimedWorld() {
 const client::Minecraft* client = client::Minecraft::INSTANCE;
 if(client == nullptr || client->world == nullptr || !client->world->celestialPhaseClaimed()) {
  return nullptr;
 }
 return client->world;
}
bool timeEnabled(const GameOptions&) {
 return claimedWorld() == nullptr;
}
std::array<OptionSpec, 4> kSpecs{{
    d::makeToggle("weather",
                  44,
                  ApplyFlags::ApplyToWorld,
                  d::getBoolMember<&GameOptions::weather>,
                  d::cycleBoolMember<&GameOptions::weather>,
                  d::loadBoolMember<&GameOptions::weather>,
                  d::saveBoolMember<&GameOptions::weather>),
    d::makeCycle("time",
                 45,
                 ApplyFlags::ApplyToWorld,
                 d::cycleIntMod<&GameOptions::time, 3>,
                 d::loadIntMember<&GameOptions::time>,
                 d::saveIntMember<&GameOptions::time>,
                 nullptr,
                 timeEnabled),
    d::makeCycle("autoSaveTicks",
                 46,
                 ApplyFlags::ApplyToWorld,
                 cycleAutoSaveTicks,
                 d::loadClampedIntMember<&GameOptions::autoSaveTicks, 40>,
                 d::saveIntMember<&GameOptions::autoSaveTicks>),
    d::makeToggle("fastDebugInfo",
                  47,
                  ApplyFlags::None,
                  d::getBoolMember<&GameOptions::fastDebugInfo>,
                  d::cycleBoolMember<&GameOptions::fastDebugInfo>,
                  d::loadBoolMember<&GameOptions::fastDebugInfo>,
                  d::saveBoolMember<&GameOptions::fastDebugInfo>),
}};
} // namespace world_screen
WorldSettingsScreen::WorldSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
    : parentFactory_(std::move(parentFactory)), gameOptions_(gameOptions), title_("World Settings") {
}
std::string_view WorldSettingsScreen::getScreenUiId() const {
 return mod::screen_ids::kWorldSettings;
}
void WorldSettingsScreen::init() {
 buttons_.clear();
 if(gameOptions_ == nullptr || minecraft() == nullptr) {
  return;
 }
 OptionGuiBuilder gui(*this, *minecraft(), *gameOptions_);
 const int x1 = gui.gridX(0);
 const int x2 = gui.gridX(1);
 const int y0 = gui.gridY(0);
 constexpr int dy = layout::kRowSpacing;
 gui.toggle(x1, y0, "weather", "Weather");
 gui.customCycle(x2, y0, "time", [](const GameOptions& o) {
  if(const net::minecraft::World* owner = world_screen::claimedWorld()) {
   const std::string& name = owner->celestialPhaseOwner();
   return optionLabel("Time", name.empty() ? std::string("mod") : name);
  }
  static constexpr std::array<const char*, 3> labels{"Default", "Day", "Night"};
  return optionLabel("Time", labels[((o.time % 3) + 3) % 3]);
 });
 gui.customCycle(x1, y0 + dy, "autoSaveTicks", [](const GameOptions& o) {
  static constexpr std::array<const char*, 4> labels{"2s", "20s", "3min", "30min"};
  for(std::size_t i = 0; i < world_screen::kAutoSaveTicks.size(); ++i) {
   if(world_screen::kAutoSaveTicks[i] == o.autoSaveTicks) {
    return optionLabel("Autosave", labels[i]);
   }
  }
  return optionLabel("Autosave", "?");
 });
 gui.toggle(x2, y0 + dy, "fastDebugInfo", "Fast Debug Info");
 layout::refreshOptionStates(buttons_, *gameOptions_);
 int footerY = layout::optionsGridY(height(), 3);
 publishScreenUi(mod::screen_regions::kFooter, &footerY);
 layout::OptionsBuildContext ctx{*this, *minecraft(), *gameOptions_};
 layout::addDoneButton(ctx, footerY, resource::language::I18n::getTranslation("gui.done"), [this] {
  if(gameOptions_ == nullptr || minecraft() == nullptr) {
   return;
  }
  gameOptions_->save();
  navigateTo(parentFactory_);
 });
}
void WorldSettingsScreen::render(int mouseX, int mouseY, float tickDelta) {
 // A mod can claim or release the clock while this screen is open (its toggle lives in
 // the footer), so re-evaluate rather than freezing whatever init() saw.
 if(gameOptions_ != nullptr) {
  layout::refreshOptionStates(buttons_, *gameOptions_);
 }
 renderBackground();
 if(textRenderer() != nullptr) {
  textRenderer()->drawCenteredWithShadow(title_, width() / 2, 20, 0xFFFFFF);
 }
 Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::gui::screen::option
