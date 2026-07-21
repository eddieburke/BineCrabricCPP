#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "net/minecraft/client/option/KeyBinding.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::gui::screen {
class Screen;
}
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft::client::input {
enum class InputLayer {
 Game,
 ScreenModal,
 ScreenPassthrough,
};
enum class SlotClickModifier : std::uint8_t {
 None = 0,
 Shift = 1,
};
struct ModifierState {
 bool shift = false;
 bool ctrl = false;
 bool alt = false;
 bool guiShift = false;
};
struct MovementState {
 float sideways = 0.0f;
 float forward = 0.0f;
 bool jumping = false;
 bool sneaking = false;
};
/// RAII input layer for screens that pass keyboard to gameplay (inventory).
class InputScope {
 public:
 explicit InputScope(InputLayer layer);
 ~InputScope();
 InputScope(const InputScope&) = delete;
 InputScope& operator=(const InputScope&) = delete;

 private:
 InputLayer layer_;
};
/// Win32 message queues, routing, movement, and mouse look — one implementation.
class InputSystem {
 public:
 static InputSystem& instance();
 void beginFrame(Minecraft& client);
 void drainScreenEvents(gui::screen::Screen& screen);
 void pollGame(Minecraft& client);
 [[nodiscard]] bool acceptsInput() const;
 [[nodiscard]] InputLayer currentLayer() const;
 [[nodiscard]] const ModifierState& modifiers() const noexcept {
  return modifiers_;
 }
 [[nodiscard]] const MovementState& movement() const noexcept {
  return movement_;
 }
 [[nodiscard]] SlotClickModifier slotClickModifier() const noexcept;
 [[nodiscard]] bool passesKeyboardToGame() const;
 void pushLayer(InputLayer layer);
 void popLayer();
 void setKeyboardRepeat(bool enabled);
 void resetBindings();
 void refreshMovement(option::GameOptions& options);
#ifdef _WIN32
 static void init(HWND window);
 static void shutdown();
 static void clearOnDeactivate();
 static void compactQueues();
 static LRESULT handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
 static void pushKeyEvent(int key, bool down);
 static void pushCharEvent(int character);
 static void pushMouseButtonEvent(int button, bool down, int x, int y);
 static void pushMouseWheelEvent(int delta, int x, int y);
 static void setCursorPosition(int x, int y);
 void initMouse(HWND parentWindow);
 void lockCursor();
 void unlockCursor();
 void pollMouseLook();
 [[nodiscard]] int mouseLookDeltaX() const noexcept {
  return mouseLookDeltaX_;
 }
 [[nodiscard]] int mouseLookDeltaY() const noexcept {
  return mouseLookDeltaY_;
 }
#endif
 [[nodiscard]] bool isMouseButtonDown(int button) const;
 [[nodiscard]] int mouseX() const noexcept;
 [[nodiscard]] int mouseY() const noexcept;
#ifdef _WIN32
 void syncCursorFromOs();
#endif
 [[nodiscard]] bool nextMouseEvent();
 [[nodiscard]] bool nextKeyboardEvent();
 [[nodiscard]] int eventMouseX() const noexcept;
 [[nodiscard]] int eventMouseY() const noexcept;
 [[nodiscard]] int eventMouseButton() const noexcept;
 [[nodiscard]] bool eventMouseButtonDown() const noexcept;
 [[nodiscard]] int eventMouseWheel() const noexcept;
 [[nodiscard]] int eventKey() const noexcept;
 [[nodiscard]] bool eventKeyDown() const noexcept;
 [[nodiscard]] char eventChar() const noexcept;
 [[nodiscard]] bool isKeyDown(int keyCode) const;
 [[nodiscard]] static bool bindingKeyMatches(int key, int bindingCode);
 [[nodiscard]] bool isBindingDown(const option::GameOptions& options, const option::KeyBinding& binding) const;

 private:
 void refreshModifiers();
 void handleKeyboardEdge(int key, bool down);
 void updateMovementKey(int key, bool down);
 void onScreenChanged(gui::screen::Screen* screen);
 void releaseCapturedKeys();
#ifdef _WIN32
 void pollGameMouse(Minecraft& client);
 void pollGameKeyboard(Minecraft& client);
 void dispatchGameKey(Minecraft& client, int key);
 void ingestKey(int vk, bool down);
 void ingestKeyCode(int key, bool down);
 void ingestChar(int character);
 void ingestMouseButton(int button, bool down, int x, int y);
 void ingestMouseWheel(int delta, int x, int y);
 void updateCursorPosition(); // beginFrame; use syncCursorFromOs() before UI hover reads
 [[nodiscard]] static int clientMouseY(HWND hwnd, int clientY);
#endif
 std::vector<InputLayer> layerStack_;
 ModifierState modifiers_;
 MovementState movement_;
 InputLayer inferredLayer_ = InputLayer::Game;
 bool guiLeftShiftIntent_ = false;
 gui::screen::Screen* activeScreen_ = nullptr;
 option::GameOptions* activeOptions_ = nullptr;
 std::array<bool, 6> movementKeys_{};
 int eventMouseX_ = 0;
 int eventMouseY_ = 0;
 int eventMouseButton_ = -1;
 bool eventMouseButtonDown_ = false;
 int eventMouseWheel_ = 0;
 int eventKey_ = 0;
 bool eventKeyDown_ = false;
 char eventChar_ = 0;
#ifdef _WIN32
 struct KeyboardEvent {
  int key = 0;
  bool down = false;
  char character = 0;
 };
 struct MouseEvent {
  int x = 0;
  int y = 0;
  int button = -1;
  bool down = false;
  int wheel = 0;
 };
 std::vector<KeyboardEvent> keyboardEvents_;
 std::array<bool, 256> keyboardDown_{};
 std::size_t keyboardReadIndex_ = 0;
 bool keyboardRepeat_ = false;
 std::vector<MouseEvent> mouseEvents_;
 std::size_t mouseReadIndex_ = 0;
 std::array<bool, 3> mouseDown_{};
 int cursorX_ = 0;
 int cursorY_ = 0;
 HWND mouseWindow_ = nullptr;
 HCURSOR mouseCursor_ = nullptr;
 bool cursorGrabbed_ = false;
 POINT mouseLastPoint_{0, 0};
 bool mouseHasLastPoint_ = false;
 int mouseLookDeltaX_ = 0;
 int mouseLookDeltaY_ = 0;
#endif
};
} // namespace net::minecraft::client::input
