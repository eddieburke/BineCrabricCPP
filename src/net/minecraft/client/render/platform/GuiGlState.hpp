#pragma once

#include "net/minecraft/client/gl/GL11.hpp"

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

    static void beginVignetteBlend() noexcept;
    static void endVignetteBlend() noexcept;

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

class ScopedNoTexture2D {
public:
    ScopedNoTexture2D() noexcept { gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D); }
    ~ScopedNoTexture2D() { gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D); }

    ScopedNoTexture2D(const ScopedNoTexture2D&) = delete;
    ScopedNoTexture2D& operator=(const ScopedNoTexture2D&) = delete;
};

class ScopedSmoothShade {
public:
    ScopedSmoothShade() noexcept { gl::GL11::glShadeModel(gl::GL11::GL_SMOOTH); }
    ~ScopedSmoothShade() { gl::GL11::glShadeModel(gl::GL11::GL_FLAT); }

    ScopedSmoothShade(const ScopedSmoothShade&) = delete;
    ScopedSmoothShade& operator=(const ScopedSmoothShade&) = delete;
};

class ScopedRescaleNormal {
public:
    ScopedRescaleNormal() noexcept { gl::GL11::glEnable(gl::GL11::GL_RESCALE_NORMAL); }
    ~ScopedRescaleNormal() { gl::GL11::glDisable(gl::GL11::GL_RESCALE_NORMAL); }

    ScopedRescaleNormal(const ScopedRescaleNormal&) = delete;
    ScopedRescaleNormal& operator=(const ScopedRescaleNormal&) = delete;
};

class ScopedProfilerDraw {
public:
    ScopedProfilerDraw() noexcept
    {
        gl::GL11::glLineWidth(1.0f);
        gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
    }
    ~ScopedProfilerDraw() { gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D); }

    ScopedProfilerDraw(const ScopedProfilerDraw&) = delete;
    ScopedProfilerDraw& operator=(const ScopedProfilerDraw&) = delete;
};

} // namespace net::minecraft::client::render::platform
