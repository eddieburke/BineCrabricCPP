#pragma once

#include "net/minecraft/client/option/GameOptions.hpp"

#include <cmath>
#include <utility>

namespace net::minecraft::client::util {

struct UiScale {
    int scaledWidth = 0;
    int scaledHeight = 0;
    double rawWidth = 0.0;
    double rawHeight = 0.0;
    int factor = 1;
};

/// Per-eye UI width in side-by-side stereo; full width otherwise.
inline int uiFramebufferWidth(const option::GameOptions& options, int displayWidth) noexcept
{
    return options.isSideBySideActive() ? displayWidth / 2 : displayWidth;
}

/// Java ScreenScaler parity from explicit framebuffer dimensions.
inline UiScale uiScale(const option::GameOptions& options, int fbWidth, int fbHeight) noexcept
{
    UiScale s;
    s.scaledWidth = fbWidth;
    s.scaledHeight = fbHeight;
    int target = options.guiScale;
    if (target == 0) {
        target = 1000;
    }
    while (s.factor < target
        && s.scaledWidth / (s.factor + 1) >= 320
        && s.scaledHeight / (s.factor + 1) >= 240) {
        ++s.factor;
    }
    s.rawWidth = static_cast<double>(s.scaledWidth) / static_cast<double>(s.factor);
    s.rawHeight = static_cast<double>(s.scaledHeight) / static_cast<double>(s.factor);
    s.scaledWidth = static_cast<int>(std::ceil(s.rawWidth));
    s.scaledHeight = static_cast<int>(std::ceil(s.rawHeight));
    return s;
}

inline std::pair<int, int> mapPhysicalMouse(
    int displayWidth,
    int displayHeight,
    int scaledWidth,
    int scaledHeight,
    int viewportX,
    int viewportHalfWidth,
    int physX,
    int physY) noexcept
{
    const int scaledX = viewportHalfWidth > 0
        ? (physX - viewportX) * scaledWidth / viewportHalfWidth
        : physX * scaledWidth / displayWidth;
    return {scaledX, scaledHeight - physY * scaledHeight / displayHeight - 1};
}

/// Map window-space cursor coords to scaled GUI coords for stereo or mono UI.
/// Side-by-side: use the half of the window that contains the cursor, not the eye being drawn.
inline std::pair<int, int> mapStereoUiMouse(
    const option::GameOptions& options,
    int displayWidth,
    int displayHeight,
    int scaledWidth,
    int scaledHeight,
    int physX,
    int physY) noexcept
{
    int viewportX = 0;
    int viewportHalfWidth = 0;
    if (options.isSideBySideActive()) {
        viewportHalfWidth = displayWidth / 2;
        if (physX >= viewportHalfWidth) {
            viewportX = viewportHalfWidth;
        }
    }
    return mapPhysicalMouse(
        displayWidth,
        displayHeight,
        scaledWidth,
        scaledHeight,
        viewportX,
        viewportHalfWidth,
        physX,
        physY);
}

inline std::pair<int, int> mapScreenMouse(
    const option::GameOptions& options,
    int displayWidth,
    int displayHeight,
    int scaledWidth,
    int scaledHeight,
    int eventX,
    int eventY) noexcept
{
    return mapStereoUiMouse(
        options, displayWidth, displayHeight, scaledWidth, scaledHeight, eventX, eventY);
}

} // namespace net::minecraft::client::util
