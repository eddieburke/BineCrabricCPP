#pragma once

namespace net::minecraft::client::render::platform {

// Centralizes 2D GUI / HUD GL state for beta 1.7.3.
// World rendering uses GlState; anything drawn in ortho HUD space uses this.
class GuiGlState {
public:
    static void setupHudProjection(double width, double height) noexcept;
    static void setupProfilerProjection(int displayWidth, int displayHeight) noexcept;

    static void disableWorldEffects() noexcept;
    static void resetColor() noexcept;

    static void enableStandardBlend() noexcept;
    static void disableBlend() noexcept;

    static void beginCrosshairBlend() noexcept;
    static void endCrosshairBlend() noexcept;

    static void beginFullscreenOverlay() noexcept;
    static void endFullscreenOverlay() noexcept;

    static void beginLitHotbarItems() noexcept;
    static void endLitHotbarItems() noexcept;

    static void beginUnlitText() noexcept;
    static void endUnlitText() noexcept;

    static void beginAlphaText() noexcept;
    static void endAlphaText() noexcept;

    static void beginSleepFade() noexcept;
    static void endSleepFade() noexcept;

    static void beginProfilerDraw() noexcept;
    static void endProfilerDraw() noexcept;
};

class ScopedLitHotbar {
public:
    ScopedLitHotbar() noexcept { GuiGlState::beginLitHotbarItems(); }
    ~ScopedLitHotbar() { GuiGlState::endLitHotbarItems(); }

    ScopedLitHotbar(const ScopedLitHotbar&) = delete;
    ScopedLitHotbar& operator=(const ScopedLitHotbar&) = delete;
};

class ScopedUnlitText {
public:
    ScopedUnlitText() noexcept { GuiGlState::beginUnlitText(); }
    ~ScopedUnlitText() { GuiGlState::endUnlitText(); }

    ScopedUnlitText(const ScopedUnlitText&) = delete;
    ScopedUnlitText& operator=(const ScopedUnlitText&) = delete;
};

class ScopedFullscreenOverlay {
public:
    ScopedFullscreenOverlay() noexcept { GuiGlState::beginFullscreenOverlay(); }
    ~ScopedFullscreenOverlay() { GuiGlState::endFullscreenOverlay(); }

    ScopedFullscreenOverlay(const ScopedFullscreenOverlay&) = delete;
    ScopedFullscreenOverlay& operator=(const ScopedFullscreenOverlay&) = delete;
};

} // namespace net::minecraft::client::render::platform
