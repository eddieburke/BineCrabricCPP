#pragma once

namespace net::minecraft::client::render::platform {

// Merges per-eye offscreen targets onto the default framebuffer.
class StereoCompositor {
public:
    // Faithful beta anaglyph: GB from eye 0, R from eye 1 (matches legacy color-mask passes).
    static void compositeAnaglyph(unsigned int eye0Texture, unsigned int eye1Texture, int displayWidth,
        int displayHeight) noexcept;

    // Side-by-side: left eye in left half, right eye in right half.
    static void compositeSideBySide(unsigned int eye0Texture, unsigned int eye1Texture, int displayWidth,
        int displayHeight) noexcept;

private:
    static void drawTexturedRect(int x, int y, int w, int h, unsigned int texture, int displayWidth,
        int displayHeight) noexcept;
};

} // namespace net::minecraft::client::render::platform
