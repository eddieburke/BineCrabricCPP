#pragma once
#include <array>
#include <cstdint>
#include <vector>
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
struct ModifierState {
 bool shift = false;
 bool ctrl = false;
 bool alt = false;
};
struct MovementState {
 float sideways = 0.0f;
 float forward = 0.0f;
 bool jumping = false;
 bool sneaking = false;
};
struct KeyboardEvent {
 int key = 0;
 bool down = false;
 char character = 0;
 bool repeat = false;
};
struct MouseEvent {
 int x = 0;
 int y = 0;
 int button = -1;
 bool down = false;
 int wheel = 0;
};
class InputSystem {
 public:
 static InputSystem& instance();
 void beginFrame(Minecraft& client);
 void drainScreenEvents(gui::screen::Screen& screen);
 void pollGame(Minecraft& client);
 [[nodiscard]] const ModifierState& modifiers() const noexcept {
  return modifiers_;
 }
 [[nodiscard]] const MovementState& movement() const noexcept {
  return movement_;
 }
 void setKeyboardRepeat(bool enabled);
 void resetBindings();
 void refreshMovement();
 void clearOnDeactivate();
 void pushKeyEvent(int key, bool down);
 void pushCharEvent(int character);
 void pushMouseButtonEvent(int button, bool down, int x, int y);
 void pushMouseWheelEvent(int delta, int x, int y);
 void setCursorPosition(int x, int y);
 void lockCursor();
 void unlockCursor();
 void pollMouseLook();
 [[nodiscard]] int mouseLookDeltaX() const noexcept {
  return mouseLookDeltaX_;
 }
 [[nodiscard]] int mouseLookDeltaY() const noexcept {
  return mouseLookDeltaY_;
 }
 [[nodiscard]] bool isMouseButtonDown(int button) const;
 [[nodiscard]] int mouseX() const noexcept;
 [[nodiscard]] int mouseY() const noexcept;
 [[nodiscard]] bool isKeyDown(int keyCode) const;

 private:
 void compactQueues();
 void refreshModifiers();
 void pollGameMouse(Minecraft& client);
 void pollGameKeyboard(Minecraft& client);
 void dispatchGameKey(Minecraft& client, int key);
 [[nodiscard]] const MouseEvent* nextMouseEvent();
 [[nodiscard]] const KeyboardEvent* nextKeyboardEvent();
 ModifierState modifiers_;
 MovementState movement_;
 option::GameOptions* activeOptions_ = nullptr;
 std::vector<KeyboardEvent> keyboardEvents_;
 std::array<bool, 256> keyboardDown_{};
 std::size_t keyboardReadIndex_ = 0;
 bool keyboardRepeat_ = false;
 std::vector<MouseEvent> mouseEvents_;
 std::size_t mouseReadIndex_ = 0;
 std::array<bool, 3> mouseDown_{};
 int cursorX_ = 0;
 int cursorY_ = 0;
 int mouseLastX_ = 0;
 int mouseLastY_ = 0;
 bool mouseHasLastPoint_ = false;
 int mouseLookDeltaX_ = 0;
 int mouseLookDeltaY_ = 0;
};
} // namespace net::minecraft::client::input
