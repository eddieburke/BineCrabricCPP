#pragma once
#include <functional>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
struct GLFWwindow;
namespace net::minecraft::client::platform::glfw {
struct DisplayMode {
  int width = 854;
  int height = 480;
  [[nodiscard]] int getWidth() const {
    return width;
  }
  [[nodiscard]] int getHeight() const {
    return height;
  }
};
/// Portable GLFW window, OpenGL context owner, event pump, and buffer present.
class Window {
public:
  using ResizeCallback = std::function<void(int width, int height)>;
  using DeactivateCallback = std::function<void()>;
  static void setParent(void* canvas);
  static void setFullscreen(bool value);
  static void setDisplayMode(const DisplayMode& mode);
  static void setTitle(const char* title);
  static void setResizeCallback(ResizeCallback callback);
  static void setDeactivateCallback(DeactivateCallback callback);
  static void create();
  static void ensureGlContext();
  static void destroy();
  static void pumpMessages();
  static void present();
  static void pumpAndPresent();
  [[nodiscard]] static bool isCloseRequested();
  [[nodiscard]] static bool isActive();
  [[nodiscard]] static bool isFullscreen();
  [[nodiscard]] static DisplayMode getDisplayMode();
  [[nodiscard]] static DisplayMode getDesktopDisplayMode();
#ifdef _WIN32
  [[nodiscard]] static HWND hwnd();
#endif
  static void notifyCloseRequested();
  static void notifyResize();
  static void setActive(bool active);

private:
  static GLFWwindow* window();
};
} // namespace net::minecraft::client::platform::glfw
