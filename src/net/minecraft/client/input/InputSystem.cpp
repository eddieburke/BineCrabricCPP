#include "net/minecraft/client/input/InputSystem.hpp"
#include <chrono>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/screen/ChatScreen.hpp"
#include "net/minecraft/client/gui/screen/ingame/InventoryScreen.hpp"
#include "net/minecraft/client/input/Keys.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/platform/glfw/Window.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
namespace net::minecraft::client::input {
namespace display = net::minecraft::client::platform::glfw;
namespace {
std::int64_t currentTimeMillis() {
 return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
     .count();
}
} // namespace
InputSystem& InputSystem::instance() {
 static InputSystem system;
 return system;
}
void InputSystem::setKeyboardRepeat(bool enabled) {
 keyboardRepeat_ = enabled;
}
void InputSystem::resetBindings() {
 movement_ = {};
}
void InputSystem::refreshMovement() {
 movement_ = {};
 if(activeOptions_ == nullptr) {
  return;
 }
 const option::GameOptions& options = *activeOptions_;
 if(isKeyDown(options.forwardKey.code)) {
  movement_.forward += 1.0f;
 }
 if(isKeyDown(options.backKey.code)) {
  movement_.forward -= 1.0f;
 }
 if(isKeyDown(options.leftKey.code)) {
  movement_.sideways += 1.0f;
 }
 if(isKeyDown(options.rightKey.code)) {
  movement_.sideways -= 1.0f;
 }
 movement_.jumping = isKeyDown(options.jumpKey.code);
 movement_.sneaking = isKeyDown(options.sneakKey.code);
 if(movement_.sneaking) {
  movement_.sideways = static_cast<float>(static_cast<double>(movement_.sideways) * 0.3);
  movement_.forward = static_cast<float>(static_cast<double>(movement_.forward) * 0.3);
 }
}
void InputSystem::clearOnDeactivate() {
 keyboardDown_.fill(false);
 keyboardEvents_.clear();
 keyboardReadIndex_ = 0;
 mouseDown_.fill(false);
 mouseEvents_.clear();
 mouseReadIndex_ = 0;
 cursorX_ = 0;
 cursorY_ = 0;
 resetBindings();
 modifiers_ = {};
}
void InputSystem::compactQueues() {
 if(keyboardReadIndex_ > 0) {
  keyboardEvents_.erase(keyboardEvents_.begin(),
                        keyboardEvents_.begin() + static_cast<std::ptrdiff_t>(keyboardReadIndex_));
  keyboardReadIndex_ = 0;
 }
 if(mouseReadIndex_ > 0) {
  mouseEvents_.erase(mouseEvents_.begin(),
                     mouseEvents_.begin() + static_cast<std::ptrdiff_t>(mouseReadIndex_));
  mouseReadIndex_ = 0;
 }
 display::Window::cursorPosition(cursorX_, cursorY_);
}
void InputSystem::pushKeyEvent(int key, bool down) {
 if(key < 0 || key >= static_cast<int>(keyboardDown_.size())) {
  return;
 }
 const std::size_t index = static_cast<std::size_t>(key);
 const bool repeat = keyboardDown_[index] == down;
 if(repeat) {
  if(!(down && keyboardRepeat_)) {
   return;
  }
 } else {
  keyboardDown_[index] = down;
 }
 keyboardEvents_.push_back({key, down, 0, repeat});
}
void InputSystem::pushCharEvent(int character) {
 if(keyboardEvents_.empty()) {
  return;
 }
 KeyboardEvent& last = keyboardEvents_.back();
 if(!last.down) {
  return;
 }
 if(character >= 0 && character <= 255) {
  last.character = static_cast<char>(character);
 }
}
void InputSystem::pushMouseButtonEvent(int button, bool down, int x, int y) {
 if(button < 0 || button >= static_cast<int>(mouseDown_.size())) {
  return;
 }
 cursorX_ = x;
 cursorY_ = y;
 const std::size_t index = static_cast<std::size_t>(button);
 if(mouseDown_[index] == down) {
  return;
 }
 mouseDown_[index] = down;
 mouseEvents_.push_back({x, y, button, down, 0});
}
void InputSystem::pushMouseWheelEvent(int delta, int x, int y) {
 if(delta == 0) {
  return;
 }
 cursorX_ = x;
 cursorY_ = y;
 mouseEvents_.push_back({x, y, -1, false, delta});
}
void InputSystem::setCursorPosition(int x, int y) {
 cursorX_ = x;
 cursorY_ = y;
}
void InputSystem::lockCursor() {
 display::Window::setCursorLocked(true);
 mouseLookDeltaX_ = 0;
 mouseLookDeltaY_ = 0;
 mouseHasLastPoint_ = false;
}
void InputSystem::unlockCursor() {
 display::Window::setCursorLocked(false);
 mouseLookDeltaX_ = 0;
 mouseLookDeltaY_ = 0;
 mouseHasLastPoint_ = false;
}
void InputSystem::pollMouseLook() {
 int x = 0;
 int y = 0;
 display::Window::cursorPosition(x, y);
 if(!mouseHasLastPoint_) {
  mouseLastX_ = x;
  mouseLastY_ = y;
  mouseHasLastPoint_ = true;
  mouseLookDeltaX_ = 0;
  mouseLookDeltaY_ = 0;
  return;
 }
 mouseLookDeltaX_ = x - mouseLastX_;
 mouseLookDeltaY_ = y - mouseLastY_;
 mouseLastX_ = x;
 mouseLastY_ = y;
}
bool InputSystem::isKeyDown(int keyCode) const {
 if(keyCode < 0 || keyCode >= 256) {
  return false;
 }
 return keyboardDown_[static_cast<std::size_t>(keyCode)];
}
bool InputSystem::isMouseButtonDown(int button) const {
 if(button < 0 || button >= static_cast<int>(mouseDown_.size())) {
  return false;
 }
 return mouseDown_[static_cast<std::size_t>(button)];
}
int InputSystem::mouseX() const noexcept {
 return cursorX_;
}
int InputSystem::mouseY() const noexcept {
 return cursorY_;
}
void InputSystem::beginFrame(Minecraft& client) {
 activeOptions_ = &client.options;
 if(!display::Window::isActive()) {
  resetBindings();
  modifiers_ = {};
  return;
 }
 compactQueues();
 refreshModifiers();
}
void InputSystem::refreshModifiers() {
 modifiers_.shift = isKeyDown(keys::kLShift) || isKeyDown(keys::kRShift);
 modifiers_.ctrl = isKeyDown(keys::kLCtrl) || isKeyDown(keys::kRCtrl);
 modifiers_.alt = isKeyDown(keys::kLAlt) || isKeyDown(keys::kRAlt);
}
const MouseEvent* InputSystem::nextMouseEvent() {
 if(mouseReadIndex_ >= mouseEvents_.size()) {
  return nullptr;
 }
 const MouseEvent& ev = mouseEvents_[mouseReadIndex_];
 ++mouseReadIndex_;
 return &ev;
}
const KeyboardEvent* InputSystem::nextKeyboardEvent() {
 if(keyboardReadIndex_ >= keyboardEvents_.size()) {
  return nullptr;
 }
 const KeyboardEvent& ev = keyboardEvents_[keyboardReadIndex_];
 ++keyboardReadIndex_;
 return &ev;
}
void InputSystem::drainScreenEvents(gui::screen::Screen& screen) {
 if(!display::Window::isActive()) {
  return;
 }
 while(const MouseEvent* event = nextMouseEvent()) {
  screen.onMouseEvent(*event);
 }
 if(!screen.passEvents) {
  while(const KeyboardEvent* event = nextKeyboardEvent()) {
   screen.onKeyboardEvent(*event);
  }
 }
}
void InputSystem::pollGame(Minecraft& client) {
 if(!display::Window::isActive()) {
  return;
 }
 if((client.currentScreen() == nullptr || client.currentScreen()->passEvents) && client.player != nullptr) {
  pollGameMouse(client);
  if(client.attackCooldown > 0) {
   --client.attackCooldown;
  }
  pollGameKeyboard(client);
  if(client.currentScreen() == nullptr) {
   if(isMouseButtonDown(0) &&
      static_cast<float>(client.ticksPlayed - client.lastClickTicks) >= client.timer.tps / 4.0f &&
      client.focused.load()) {
    if(!net::minecraft::mod::runtime::luaHookMouseButton(0, true)) {
     client.handleMouseClick(0);
    }
    client.lastClickTicks = client.ticksPlayed;
   }
   if(isMouseButtonDown(1) &&
      static_cast<float>(client.ticksPlayed - client.lastClickTicks) >= client.timer.tps / 4.0f &&
      client.focused.load()) {
    client.handleMouseClick(1);
    client.lastClickTicks = client.ticksPlayed;
   }
  }
  client.handleMouseDown(0, client.currentScreen() == nullptr && isMouseButtonDown(0) && client.focused.load());
  refreshMovement();
 }
}
void InputSystem::pollGameMouse(Minecraft& client) {
 while(const MouseEvent* event = nextMouseEvent()) {
  if(currentTimeMillis() - client.lastTickTime > 200) {
   continue;
  }
  if(event->button >= 0 && net::minecraft::mod::runtime::luaHookMouseButton(event->button, event->down)) {
   continue;
  }
  const int wheel = event->wheel;
  if(wheel != 0) {
   client.player->inventory.scrollInHotbar(wheel);
   if(client.options.discreteScroll) {
    int discrete = wheel;
    if(discrete > 0) {
     discrete = 1;
    }
    if(discrete < 0) {
     discrete = -1;
    }
    client.options.totalDiscreteScroll += static_cast<float>(discrete) * 0.25f;
   }
  }
  if(client.currentScreen() == nullptr) {
   if(!client.focused.load() && event->down) {
    client.lockMouse();
    continue;
   }
   if(event->button == 0 && event->down) {
    client.handleMouseClick(0);
    client.lastClickTicks = client.ticksPlayed;
   }
   if(event->button == 1 && event->down) {
    client.handleMouseClick(1);
    client.lastClickTicks = client.ticksPlayed;
   }
   if(event->button == 2 && event->down) {
    client.handlePickBlock();
   }
  }
 }
}
void InputSystem::pollGameKeyboard(Minecraft& client) {
 while(const KeyboardEvent* event = nextKeyboardEvent()) {
  if(net::minecraft::mod::runtime::luaHookKeyPress(event->key, event->down, event->repeat)) {
   continue;
  }
  if(!event->down) {
   continue;
  }
  if(event->key == keys::kF11) {
   client.toggleFullscreen();
   continue;
  }
  if(client.currentScreen() != nullptr) {
   client.currentScreen()->onKeyboardEvent(*event);
   if(const int slot = keys::hotbarSlotFromKey(event->key); slot >= 0) {
    client.player->inventory.selectedSlot = slot;
   }
   if(event->key == static_cast<int>(client.options.fogKey.code)) {
    client.options.cycle("viewDistance", modifiers().shift ? -1 : 1);
   }
  } else {
   dispatchGameKey(client, event->key);
  }
 }
}
void InputSystem::dispatchGameKey(Minecraft& client, int key) {
 using option::GameOptions;
 struct ToggleKey {
  int code;
  bool GameOptions::* flag;
 };
 static constexpr ToggleKey kToggles[] = {
     {keys::kF1, &GameOptions::hideHud},
     {keys::kF3, &GameOptions::debugHud},
     {keys::kF5, &GameOptions::thirdPerson},
     {keys::kF8, &GameOptions::cinematicMode},
     {keys::kF7, &GameOptions::shadowDisablePolyOffset},
 };
 if(key == keys::kEscape) {
  client.pauseGame();
  return;
 }
 if(key == keys::kS && isKeyDown(keys::kF3)) {
  client.forceResourceReload();
 }
 for(const ToggleKey& toggle : kToggles) {
  if(key == toggle.code) {
   client.options.*toggle.flag = !(client.options.*toggle.flag);
  }
 }
 if(key == static_cast<int>(client.options.inventoryKey.code) && client.player != nullptr) {
  client.setScreen(std::make_unique<gui::screen::ingame::InventoryScreen>(&client.player->playerScreenHandler));
 }
 if(key == static_cast<int>(client.options.dropKey.code)) {
  client.player->dropSelectedItem();
 }
 if(client.isWorldRemote() && key == static_cast<int>(client.options.chatKey.code)) {
  client.setScreen(std::make_unique<gui::screen::ChatScreen>());
 }
 if(const int slot = keys::hotbarSlotFromKey(key); slot >= 0) {
  client.player->inventory.selectedSlot = slot;
 }
 if(key == static_cast<int>(client.options.fogKey.code)) {
  client.options.cycle("viewDistance", modifiers().shift ? -1 : 1);
 }
}
} // namespace net::minecraft::client::input
