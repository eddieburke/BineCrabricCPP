#include "net/minecraft/client/core/ScreenStack.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/screen/DeathScreen.hpp"
#include "net/minecraft/client/gui/screen/FatalErrorScreen.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/screen/SleepingChatScreen.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
namespace net::minecraft::client::core {
ScreenStack::ScreenStack(Minecraft& client) noexcept : client_(client) {
}
void ScreenStack::lockMouseWithoutClosingScreen() {
 if(!input::InputSystem::instance().acceptsInput()) {
  return;
 }
 if(client_.focused.load()) {
  return;
 }
 client_.focused = true;
 input::InputSystem::instance().lockCursor();
}
void ScreenStack::setScreen(std::unique_ptr<gui::screen::Screen> screen) {
 if(dynamic_cast<gui::screen::FatalErrorScreen*>(currentScreen_.get()) != nullptr) {
  return;
 }
 if(currentScreen_ != nullptr) {
  currentScreen_->removed();
 }
 if(dynamic_cast<gui::screen::TitleScreen*>(screen.get()) != nullptr && client_.stats != nullptr) {
  client_.stats->syncStats();
 }
 if(client_.stats != nullptr) {
  client_.stats->save();
 }
 if(screen == nullptr && client_.world == nullptr) {
  screen = std::make_unique<gui::screen::TitleScreen>();
 } else if(screen == nullptr && client_.player != nullptr && client_.player->health <= 0) {
  screen = std::make_unique<gui::screen::DeathScreen>();
 }
 if(dynamic_cast<gui::screen::TitleScreen*>(screen.get()) != nullptr) {
  client_.inGameHud.clearChat();
 }
 // Park the outgoing screen instead of destroying it inline: setScreen is often
 // called from within the current screen's own mouseClicked/tickInput, so freeing
 // it here would be a use-after-free as the stack unwinds. Freed in run loop.
 if(currentScreen_ != nullptr) {
  retiredScreens_.push_back(std::move(currentScreen_));
 }
 currentScreen_ = std::move(screen);
 if(currentScreen_ != nullptr) {
  client_.unlockMouse();
  const auto& options = client_.options;
  const util::UiScale scale = util::uiScale(options, client_.displayWidth, client_.displayHeight);
  currentScreen_->init(&client_, scale.scaledWidth, scale.scaledHeight);
  client_.skipGameRender = false;
 } else {
  lockMouseWithoutClosingScreen();
 }
}
void ScreenStack::flushRetired() {
 // Safe point: free screens retired during the previous frame's input.
 retiredScreens_.clear();
}
void ScreenStack::tickScreens(Minecraft& client) {
 if(currentScreen_.get() == nullptr && client.player != nullptr) {
  if(client.player->health <= 0) {
   client.setScreen(nullptr);
  } else if(client.player->isSleeping() && client.world != nullptr && client.world->isRemote()) {
   client.setScreen(std::make_unique<gui::screen::SleepingChatScreen>());
  }
 } else if(currentScreen_.get() != nullptr && client.player != nullptr &&
           dynamic_cast<gui::screen::SleepingChatScreen*>(currentScreen_.get()) != nullptr &&
           !client.player->isSleeping()) {
  client.setScreen(nullptr);
 }
 if(currentScreen_ != nullptr) {
  client.attackCooldown = 10000;
  client.lastClickTicks = client.ticksPlayed + 10000;
  currentScreen_->tickInput();
  if(currentScreen_ != nullptr) {
   currentScreen_->tick();
  }
 }
}
} // namespace net::minecraft::client::core
