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

inline std::pair<int, int> mapScreenMouse(
    const option::GameOptions& options,
    int displayWidth,
    int displayHeight,
    int scaledWidth,
    int scaledHeight,
    int eventX,
    int eventY) noexcept
{
    const int uiWidth = uiFramebufferWidth(options, displayWidth);
    if (options.isSideBySideActive()) {
        eventX %= displayWidth / 2;
    }
    return {
        eventX * scaledWidth / uiWidth,
        scaledHeight - eventY * scaledHeight / displayHeight - 1,
    };
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

} // namespace net::minecraft::client::util
