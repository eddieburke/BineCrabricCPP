#include "net/minecraft/client/gui/screen/GameMenuScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/AchievementsScreen.hpp"
#include "net/minecraft/client/gui/screen/StatsScreen.hpp"
#include "net/minecraft/client/gui/screen/option/OptionsScreen.hpp"
#include "net/minecraft/client/host/ServerProcessCoordinator.hpp"
#include "net/minecraft/client/host/LanInfoScreen.hpp"
#include "net/minecraft/client/host/LanScreen.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
namespace net::minecraft::client::gui::screen {
void GameMenuScreen::init() {
 saveStep_ = 0;
 ticks_ = 0;
 savingLevel_ = true;
 buttons_.clear();
 const int w = width();
 const int h = height();
 std::string quitText = "Save and quit to title";
 if(minecraft() != nullptr && minecraft()->isWorldRemote()) {
  quitText = "Disconnect";
 }
 addCenteredActionButton(
     layout::gameMenuButtonY(h, layout::kGameMenuQuitOffset), quitText, [this] { saveAndQuit(); });
 addCenteredActionButton(
     layout::gameMenuButtonY(h, layout::kGameMenuBackOffset), "Back to game", [this] { resumeGame(); });
 addCenteredActionButton(
     layout::gameMenuButtonY(h, layout::kGameMenuOptionsOffset), "Options...", [this] { openOptions(); });
 addActionButton(layout::centerBtnX(w),
                 layout::gameMenuButtonY(h, layout::kGameMenuSplitOffset),
                 layout::kSplitButtonWidth,
                 layout::kDefaultButtonHeight,
                 "Achievements",
                 [this] { openAchievements(); });
 addActionButton(layout::centerBtnX(w) + layout::kSplitButtonWidth + 4,
                 layout::gameMenuButtonY(h, layout::kGameMenuSplitOffset),
                 layout::kSplitButtonWidth,
                 layout::kDefaultButtonHeight,
                 "Stats",
                 [this] { openStats(); });
 if(minecraft() != nullptr && minecraft()->world != nullptr) {
  if(minecraft()->serverProcessCoordinator().isHostedWorld(minecraft()->world)) {
   addCenteredActionButton(
       layout::gameMenuButtonY(h, layout::kGameMenuServerOffset), "Server Info...", [this] { openLanInfo(); });
  } else if(minecraft()->serverProcessCoordinator().canStartServer()) {
   addCenteredActionButton(
       layout::gameMenuButtonY(h, layout::kGameMenuServerOffset), "Start Dedicated Server...", [this] { openLanMenu(); });
  }
 }
}
void GameMenuScreen::tick() {
 Screen::tick();
 ++ticks_;
 if(minecraft() != nullptr && minecraft()->world != nullptr) {
  savingLevel_ = !minecraft()->world->attemptSaving(saveStep_++);
 } else {
  savingLevel_ = true;
 }
}
void GameMenuScreen::render(int mouseX, int mouseY, float tickDelta) {
 renderBackground();
 if(savingLevel_ || ticks_ < 20) {
  float pulse = (static_cast<float>(ticks_ % 10) + tickDelta) / 10.0f;
  pulse = MathHelper::sin(pulse * static_cast<float>(3.14159265) * 2.0f) * 0.2f + 0.8f;
  const int brightness = static_cast<int>(255.0f * pulse);
  const int color = (brightness << 16) | (brightness << 8) | brightness;
  if(textRenderer() != nullptr) {
   drawTextWithShadow(*textRenderer(), "Saving level..", 8, height() - 16, color);
  }
 }
 if(textRenderer() != nullptr) {
  drawCenteredTextWithShadow(*textRenderer(), "Game menu", width() / 2, 40, 0xFFFFFF);
 }
 Screen::render(mouseX, mouseY, tickDelta);
}
void GameMenuScreen::saveAndQuit() {
 if(minecraft() == nullptr) {
  return;
 }
 if(minecraft()->stats != nullptr) {
  minecraft()->stats->increment(stat::Stats::LEAVE_GAME, 1);
 }
 if(minecraft()->isWorldRemote()) {
  if(auto* clientWorld = dynamic_cast<net::minecraft::ClientWorld*>(minecraft()->world)) {
   clientWorld->disconnect();
  }
 }
 quitToTitle();
}
void GameMenuScreen::resumeGame() {
 closeScreen();
 if(minecraft() != nullptr) {
  minecraft()->lockMouse();
 }
}
void GameMenuScreen::openLanMenu() {
 navigateTo(std::make_unique<client::host::LanScreen>());
}
void GameMenuScreen::openLanInfo() {
 navigateTo(std::make_unique<client::host::LanInfoScreen>());
}
void GameMenuScreen::openOptions() {
 if(minecraft() == nullptr) {
  return;
 }
 const auto returnToGameMenu = []() { return std::make_unique<GameMenuScreen>(); };
 navigateTo(std::make_unique<option::OptionsScreen>(returnToGameMenu, &minecraft()->options));
}
void GameMenuScreen::openAchievements() {
 if(minecraft() != nullptr && minecraft()->stats != nullptr) {
  navigateTo(std::make_unique<AchievementsScreen>(minecraft()->stats));
 }
}
void GameMenuScreen::openStats() {
 if(minecraft() == nullptr) {
  return;
 }
 const auto returnToGameMenu = []() { return std::make_unique<GameMenuScreen>(); };
 navigateTo(std::make_unique<StatsScreen>(returnToGameMenu, minecraft()->stats));
}
} // namespace net::minecraft::client::gui::screen
