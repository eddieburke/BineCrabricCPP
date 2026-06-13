#pragma once

#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::util {

/// Display bootstrap, resize, fullscreen, GL context, and present.
/// Anchor: Minecraft.cpp L154–201, L372–380, L699–768, L743–746.
class DisplayManager {
public:
    static void setupAndCreateDisplay(Minecraft& client);
    static void toggleFullscreen(Minecraft& client);
    static void resize(Minecraft& client, int width, int height);
    static void scheduleScreenResize(Minecraft& client);
    static void logGlError(Minecraft& client, const std::string& phase);

#ifdef _WIN32
    static void ensureGlContext();
    static void present();
    static void pumpAndPresent();
    static void destroy();
    [[nodiscard]] static HWND hwnd();
    [[nodiscard]] static bool isActive();
    [[nodiscard]] static bool isCloseRequested();
#endif
};

} // namespace net::minecraft::client::util
