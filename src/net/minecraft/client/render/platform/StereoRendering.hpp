#pragma once

#include <functional>

namespace net::minecraft::client::option {
class GameOptions;
}

namespace net::minecraft::client::render::platform {

struct StereoUiEyeContext {
    int eye = 0;
    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    int scaledMouseX = 0;
    int scaledMouseY = 0;
    int scaledWidth = 0;
    int scaledHeight = 0;
    double rawScaledWidth = 0.0;
    double rawScaledHeight = 0.0;

    void setupHudProjection() const noexcept;
};

// Stereo camera offsets, viewport sizing, and per-eye HUD/UI passes.
// Compositing and per-eye FBO binding live in StereoSession / StereoCompositor.
class StereoRendering {
public:
    [[nodiscard]] static bool needsSecondEye(const option::GameOptions& options) noexcept;

    static void applyProjectionStereoOffset(int eye, const option::GameOptions& options) noexcept;
    static void applyModelViewStereoOffset(int eye, const option::GameOptions& options) noexcept;

    // Legacy fallback when GL FBO extensions are unavailable.
    static void beginEyePass(const option::GameOptions& options, int eye, int displayWidth, int displayHeight) noexcept;
    static void endFrame(const option::GameOptions& options, int displayWidth, int displayHeight) noexcept;
    static void applyTranslucentDepthMask(const option::GameOptions& options) noexcept;
};

// RAII side-by-side UI pass: sets per-eye viewport and UiScale, restores full viewport on destroy.
// Anaglyph and off modes render once at full framebuffer size (Java parity).
class StereoUiFrame {
public:
    using EyeRenderFn = std::function<void(const StereoUiEyeContext& ctx)>;

    StereoUiFrame(const option::GameOptions& options, int displayWidth, int displayHeight) noexcept;
    ~StereoUiFrame();

    void forEachEye(const EyeRenderFn& renderFn) noexcept;

    StereoUiFrame(const StereoUiFrame&) = delete;
    StereoUiFrame& operator=(const StereoUiFrame&) = delete;

private:
    const option::GameOptions& options_;
    int displayWidth_;
    int displayHeight_;
    bool sideBySide_ = false;
};

} // namespace net::minecraft::client::render::platform
