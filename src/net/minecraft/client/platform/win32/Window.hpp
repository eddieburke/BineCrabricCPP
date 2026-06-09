#pragma once

#include <functional>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace net::minecraft::client::platform::win32 {

#ifdef _WIN32

struct DisplayMode {
    int width = 854;
    int height = 480;
    [[nodiscard]] int getWidth() const { return width; }
    [[nodiscard]] int getHeight() const { return height; }
};

/// Win32 window, WGL context owner, message pump, and buffer present.
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
    [[nodiscard]] static HWND hwnd();

    static void notifyCloseRequested();
    static void notifyResize();
    static void setActive(bool active);

private:
    static void applyWindowedClientSize(int width, int height);
    static void enterBorderlessFullscreen();
    static void exitBorderlessFullscreen();
};

#endif // _WIN32

} // namespace net::minecraft::client::platform::win32
