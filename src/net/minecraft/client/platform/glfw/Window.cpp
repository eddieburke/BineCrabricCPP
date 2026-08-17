#include "net/minecraft/client/platform/glfw/Window.hpp"
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
namespace net::minecraft::client::platform::glfw {
namespace {
bool fullscreen_ = false;
bool closeRequested_ = false;
bool active_ = true;
bool visible_ = true;
bool initialized_ = false;
int pendingWidth_ = 854;
int pendingHeight_ = 480;
int windowedX_ = 100;
int windowedY_ = 100;
int windowedWidth_ = 854;
int windowedHeight_ = 480;
std::string title_ = "Minecraft Beta 1.7.3";
GLFWwindow* window_ = nullptr;
Window::ResizeCallback resizeCallback_{};
Window::DeactivateCallback deactivateCallback_{};
int glfwKeyToMinecraftKey(int key) {
 if(key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
  return 2 + (key - GLFW_KEY_1);
 }
 if(key == GLFW_KEY_0) {
  return 11;
 }
 if(key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
  static constexpr int kKeys[] = {
      30,
      48,
      46,
      32,
      18,
      33,
      34,
      35,
      23,
      36,
      37,
      38,
      50,
      49,
      24,
      25,
      16,
      19,
      31,
      20,
      22,
      47,
      17,
      45,
      21,
      44,
  };
  return kKeys[key - GLFW_KEY_A];
 }
 if(key >= GLFW_KEY_F1 && key <= GLFW_KEY_F10) {
  return 59 + (key - GLFW_KEY_F1);
 }
 if(key == GLFW_KEY_F11) {
  return 87;
 }
 if(key == GLFW_KEY_F12) {
  return 88;
 }
 if(key >= GLFW_KEY_F13 && key <= GLFW_KEY_F15) {
  return 100 + (key - GLFW_KEY_F13);
 }
 switch(key) {
 case GLFW_KEY_ESCAPE:
  return 1;
 case GLFW_KEY_MINUS:
  return 12;
 case GLFW_KEY_EQUAL:
  return 13;
 case GLFW_KEY_BACKSPACE:
  return 14;
 case GLFW_KEY_TAB:
  return 15;
 case GLFW_KEY_LEFT_BRACKET:
  return 26;
 case GLFW_KEY_RIGHT_BRACKET:
  return 27;
 case GLFW_KEY_ENTER:
  return 28;
 case GLFW_KEY_LEFT_CONTROL:
  return 29;
 case GLFW_KEY_SEMICOLON:
  return 39;
 case GLFW_KEY_APOSTROPHE:
  return 40;
 case GLFW_KEY_GRAVE_ACCENT:
  return 41;
 case GLFW_KEY_LEFT_SHIFT:
  return 42;
 case GLFW_KEY_BACKSLASH:
  return 43;
 case GLFW_KEY_COMMA:
  return 51;
 case GLFW_KEY_PERIOD:
  return 52;
 case GLFW_KEY_SLASH:
  return 53;
 case GLFW_KEY_RIGHT_SHIFT:
  return 54;
 case GLFW_KEY_KP_MULTIPLY:
  return 55;
 case GLFW_KEY_LEFT_ALT:
  return 56;
 case GLFW_KEY_SPACE:
  return 57;
 case GLFW_KEY_CAPS_LOCK:
  return 58;
 case GLFW_KEY_NUM_LOCK:
  return 69;
 case GLFW_KEY_SCROLL_LOCK:
  return 70;
 case GLFW_KEY_KP_7:
  return 71;
 case GLFW_KEY_KP_8:
  return 72;
 case GLFW_KEY_KP_9:
  return 73;
 case GLFW_KEY_KP_SUBTRACT:
  return 74;
 case GLFW_KEY_KP_4:
  return 75;
 case GLFW_KEY_KP_5:
  return 76;
 case GLFW_KEY_KP_6:
  return 77;
 case GLFW_KEY_KP_ADD:
  return 78;
 case GLFW_KEY_KP_1:
  return 79;
 case GLFW_KEY_KP_2:
  return 80;
 case GLFW_KEY_KP_3:
  return 81;
 case GLFW_KEY_KP_0:
  return 82;
 case GLFW_KEY_KP_DECIMAL:
  return 83;
 case GLFW_KEY_KP_ENTER:
  return 156;
 case GLFW_KEY_UP:
  return 200;
 case GLFW_KEY_DOWN:
  return 208;
 case GLFW_KEY_RIGHT_CONTROL:
  return 157;
 case GLFW_KEY_KP_DIVIDE:
  return 181;
 case GLFW_KEY_PRINT_SCREEN:
  return 183;
 case GLFW_KEY_RIGHT_ALT:
  return 184;
 case GLFW_KEY_PAUSE:
  return 197;
 case GLFW_KEY_HOME:
  return 199;
 case GLFW_KEY_PAGE_UP:
  return 201;
 case GLFW_KEY_LEFT:
  return 203;
 case GLFW_KEY_RIGHT:
  return 205;
 case GLFW_KEY_END:
  return 207;
 case GLFW_KEY_PAGE_DOWN:
  return 209;
 case GLFW_KEY_INSERT:
  return 210;
 case GLFW_KEY_DELETE:
  return 211;
 case GLFW_KEY_LEFT_SUPER:
  return 219;
 case GLFW_KEY_RIGHT_SUPER:
  return 220;
 case GLFW_KEY_MENU:
  return 221;
 default:
  return -1;
 }
}
void applyCallbacks(GLFWwindow* window) {
 glfwSetWindowCloseCallback(window, [](GLFWwindow*) { Window::notifyCloseRequested(); });
 glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int, int) { Window::notifyResize(); });
 glfwSetWindowFocusCallback(window, [](GLFWwindow*, int focused) { Window::setActive(focused == GLFW_TRUE); });
 glfwSetKeyCallback(window, [](GLFWwindow*, int key, int, int action, int) {
  if(action != GLFW_PRESS && action != GLFW_RELEASE && action != GLFW_REPEAT) {
   return;
  }
  const int minecraftKey = glfwKeyToMinecraftKey(key);
  if(minecraftKey >= 0) {
   input::InputSystem::instance().pushKeyEvent(minecraftKey, action != GLFW_RELEASE);
  }
 });
 glfwSetCharCallback(window, [](GLFWwindow*, unsigned int codepoint) {
  input::InputSystem::instance().pushCharEvent(static_cast<int>(codepoint));
 });
 glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, int) {
  if(button < GLFW_MOUSE_BUTTON_LEFT || button > GLFW_MOUSE_BUTTON_MIDDLE) {
   return;
  }
  int x = 0;
  int y = 0;
  Window::cursorPosition(x, y);
  input::InputSystem::instance().pushMouseButtonEvent(button, action == GLFW_PRESS, x, y);
 });
 glfwSetScrollCallback(window, [](GLFWwindow*, double, double yoffset) {
  int x = 0;
  int y = 0;
  Window::cursorPosition(x, y);
  const int delta = static_cast<int>(std::round(yoffset * 120.0));
  input::InputSystem::instance().pushMouseWheelEvent(delta, x, y);
 });
 glfwSetCursorPosCallback(window, [](GLFWwindow*, double xpos, double ypos) {
  int width = 0;
  int height = 0;
  glfwGetWindowSize(window_, &width, &height);
  input::InputSystem::instance().setCursorPosition(static_cast<int>(std::floor(xpos)),
                                                   height - static_cast<int>(std::floor(ypos)));
 });
}
GLFWmonitor* primaryMonitor() {
 GLFWmonitor* monitor = glfwGetPrimaryMonitor();
 if(monitor == nullptr) {
  throw std::runtime_error("glfwGetPrimaryMonitor failed");
 }
 return monitor;
}
const GLFWvidmode* primaryVideoMode() {
 return glfwGetVideoMode(primaryMonitor());
}
void resetSwapPacing() {
 gl::GLCore::resetSwapPacingCache();
}
void resolveFullscreenSize() {
 const GLFWvidmode* mode = primaryVideoMode();
 if(mode != nullptr) {
  pendingWidth_ = pendingWidth_ > 0 ? pendingWidth_ : mode->width;
  pendingHeight_ = pendingHeight_ > 0 ? pendingHeight_ : mode->height;
 }
}
void applyDisplaySize(int x, int y, int width, int height, int refreshRate) {
 if(fullscreen_) {
  if(refreshRate == GLFW_DONT_CARE) {
   const GLFWvidmode* mode = primaryVideoMode();
   refreshRate = mode != nullptr ? mode->refreshRate : GLFW_DONT_CARE;
  }
  glfwSetWindowMonitor(window_, primaryMonitor(), 0, 0, width, height, refreshRate);
 } else {
  glfwSetWindowSize(window_, width, height);
  glfwSetWindowPos(window_, x, y);
 }
}
void rememberWindowedPlacement() {
 if(window_ == nullptr || fullscreen_) {
  return;
 }
 glfwGetWindowPos(window_, &windowedX_, &windowedY_);
 glfwGetWindowSize(window_, &windowedWidth_, &windowedHeight_);
}
} // namespace
void Window::cursorPosition(int& x, int& y) {
 x = 0;
 y = 0;
 if(window_ == nullptr) {
  return;
 }
 double cursorX = 0.0;
 double cursorY = 0.0;
 int width = 0;
 int height = 0;
 glfwGetCursorPos(window_, &cursorX, &cursorY);
 glfwGetWindowSize(window_, &width, &height);
 x = static_cast<int>(std::floor(cursorX));
 y = height - static_cast<int>(std::floor(cursorY));
}
void Window::setCursorLocked(bool locked) {
 if(window_ == nullptr) {
  return;
 }
 glfwSetInputMode(window_, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}
std::string Window::clipboardString() {
 if(window_ == nullptr) {
  return {};
 }
 const char* text = glfwGetClipboardString(window_);
 return text != nullptr ? text : "";
}
void Window::setResizeCallback(ResizeCallback callback) {
 resizeCallback_ = std::move(callback);
}
void Window::setDeactivateCallback(DeactivateCallback callback) {
 deactivateCallback_ = std::move(callback);
}
void Window::setFullscreen(bool value) {
 if(fullscreen_ == value) {
  return;
 }
 if(window_ == nullptr) {
  fullscreen_ = value;
  return;
 }
 if(value) {
  rememberWindowedPlacement();
  resolveFullscreenSize();
  fullscreen_ = true;
  applyDisplaySize(0, 0, pendingWidth_, pendingHeight_, GLFW_DONT_CARE);
 } else {
  fullscreen_ = false;
  applyDisplaySize(windowedX_, windowedY_, windowedWidth_ > 0 ? windowedWidth_ : pendingWidth_,
                   windowedHeight_ > 0 ? windowedHeight_ : pendingHeight_, 0);
 }
 resetSwapPacing();
 notifyResize();
}
void Window::setDisplayMode(const DisplayMode& mode) {
 pendingWidth_ = mode.width > 0 ? mode.width : pendingWidth_;
 pendingHeight_ = mode.height > 0 ? mode.height : pendingHeight_;
 if(window_ == nullptr) {
  return;
 }
 if(fullscreen_) {
  applyDisplaySize(0, 0, pendingWidth_, pendingHeight_, GLFW_DONT_CARE);
 } else {
  rememberWindowedPlacement();
  windowedWidth_ = pendingWidth_;
  windowedHeight_ = pendingHeight_;
  applyDisplaySize(windowedX_, windowedY_, pendingWidth_, pendingHeight_, 0);
 }
 notifyResize();
}
void Window::setTitle(const char* title) {
 if(title == nullptr) {
  return;
 }
 title_ = title;
 if(window_ != nullptr) {
  glfwSetWindowTitle(window_, title_.c_str());
 }
}
void Window::setVisible(bool visible) {
 visible_ = visible;
 if(window_ != nullptr) {
  if(visible_) {
   glfwShowWindow(window_);
  } else {
   glfwHideWindow(window_);
  }
 }
}
void Window::notifyResize() {
 if(window_ == nullptr || !resizeCallback_) {
  return;
 }
 int width = 0;
 int height = 0;
 glfwGetFramebufferSize(window_, &width, &height);
 if(width <= 0 || height <= 0) {
  return;
 }
 pendingWidth_ = width;
 pendingHeight_ = height;
 resizeCallback_(width, height);
}
void Window::notifyCloseRequested() {
 closeRequested_ = true;
}
void Window::setActive(bool active) {
 active_ = active;
 if(!active) {
  input::InputSystem::instance().clearOnDeactivate();
  if(deactivateCallback_) {
   deactivateCallback_();
  }
 }
}
void Window::create() {
 if(!initialized_) {
  if(glfwInit() != GLFW_TRUE) {
   throw std::runtime_error("glfwInit failed");
  }
  initialized_ = true;
 }
 if(window_ != nullptr) {
  return;
 }
 glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
 glfwWindowHint(GLFW_DEPTH_BITS, 24);
 glfwWindowHint(GLFW_STENCIL_BITS, 8);
 glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
 glfwWindowHint(GLFW_VISIBLE, visible_ ? GLFW_TRUE : GLFW_FALSE);
 glfwWindowHint(GLFW_ALPHA_BITS, 0);
 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
 glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
 GLFWmonitor* monitor = fullscreen_ ? primaryMonitor() : nullptr;
 if(fullscreen_) {
  resolveFullscreenSize();
  const GLFWvidmode* mode = primaryVideoMode();
  if(mode != nullptr) {
   glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
  }
 }
 window_ = glfwCreateWindow(pendingWidth_, pendingHeight_, title_.c_str(), monitor, nullptr);
 if(window_ == nullptr) {
  throw std::runtime_error("glfwCreateWindow failed — OpenGL 4.3 core required");
 }
 applyCallbacks(window_);
 glfwMakeContextCurrent(window_);
 resetSwapPacing();
 if(visible_) {
  glfwShowWindow(window_);
 }
 notifyResize();
}
void Window::ensureGlContext() {
 if(window_ == nullptr) {
  return;
 }
 glfwMakeContextCurrent(window_);
}
void Window::destroy() {
 if(window_ != nullptr) {
  glfwDestroyWindow(window_);
  window_ = nullptr;
 }
 gl::GLCore::resetSwapPacingCache();
 if(initialized_) {
  glfwTerminate();
  initialized_ = false;
 }
}
void Window::pumpMessages() {
 glfwPollEvents();
}
void Window::present() {
 if(window_ == nullptr) {
  return;
 }
 gl::GLCore::ensureLoaded();
 glfwSwapBuffers(window_);
}
bool Window::isCloseRequested() {
 return closeRequested_;
}
bool Window::isActive() {
 return active_;
}
DisplayMode Window::getDisplayMode() {
 if(window_ == nullptr) {
  return {pendingWidth_, pendingHeight_};
 }
 int width = 0;
 int height = 0;
 glfwGetFramebufferSize(window_, &width, &height);
 return {width, height};
}
DisplayMode Window::getDesktopDisplayMode() {
 if(!initialized_) {
  if(glfwInit() != GLFW_TRUE) {
   return {};
  }
  initialized_ = true;
 }
 const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor());
 if(mode == nullptr) {
  return {};
 }
 return {mode->width, mode->height};
}
} // namespace net::minecraft::client::platform::glfw
