#pragma once
#include <functional>
#include <string>
namespace net::minecraft::client::platform::glfw {
struct DisplayMode {
 int width = 854;
 int height = 480;
};
class Window {
 public:
 using ResizeCallback = std::function<void(int width, int height)>;
 using DeactivateCallback = std::function<void()>;
 static void setFullscreen(bool value);
 static void setDisplayMode(const DisplayMode& mode);
 static void setTitle(const char* title);
 static void setVisible(bool visible);
 static void setResizeCallback(ResizeCallback callback);
 static void setDeactivateCallback(DeactivateCallback callback);
 static void create();
 static void ensureGlContext();
 static void destroy();
 static void pumpMessages();
 static void present();
 [[nodiscard]] static bool isCloseRequested();
 [[nodiscard]] static bool isActive();
 [[nodiscard]] static DisplayMode getDisplayMode();
 [[nodiscard]] static DisplayMode getDesktopDisplayMode();
 static void setCursorLocked(bool locked);
 static void cursorPosition(int& x, int& y);
 [[nodiscard]] static std::string clipboardString();
 static void notifyCloseRequested();
 static void notifyResize();
 static void setActive(bool active);
};
} // namespace net::minecraft::client::platform::glfw
