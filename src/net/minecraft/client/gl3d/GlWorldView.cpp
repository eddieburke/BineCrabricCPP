#ifdef _WIN32

// terrain.png -> GL texture upload. Isolated here so the GlWorldView header does
// not pull <gdiplus.h> into every TU that renders.

#include <windows.h>
#include <gdiplus.h>
#include <GL/gl.h>

#include "net/minecraft/client/gl3d/GlWorldView.hpp"

namespace net::minecraft::client::gl3d {

void GlWorldView::uploadTerrain(const Gdiplus::Bitmap& bitmap)
{
    context_.makeCurrent();

    auto& mutableBitmap = const_cast<Gdiplus::Bitmap&>(bitmap);
    const int w = static_cast<int>(mutableBitmap.GetWidth());
    const int h = static_cast<int>(mutableBitmap.GetHeight());
    terrainW_ = w;
    terrainH_ = h;

    Gdiplus::Rect rect(0, 0, w, h);
    Gdiplus::BitmapData data{};
    if (mutableBitmap.LockBits(&rect, Gdiplus::ImageLockModeRead,
                               PixelFormat32bppARGB, &data) != Gdiplus::Ok) {
        return;
    }

    // Gdiplus gives BGRA (ARGB little-endian). Convert to RGBA for GL.
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(w) * h * 4);
    const auto* src = static_cast<const std::uint8_t*>(data.Scan0);
    for (int y = 0; y < h; ++y) {
        const std::uint8_t* row = src + static_cast<std::size_t>(y) * data.Stride;
        for (int x = 0; x < w; ++x) {
            const std::uint8_t b = row[x * 4 + 0];
            const std::uint8_t g = row[x * 4 + 1];
            const std::uint8_t r = row[x * 4 + 2];
            const std::uint8_t a = row[x * 4 + 3];
            const std::size_t o = (static_cast<std::size_t>(y) * w + x) * 4;
            rgba[o + 0] = r;
            rgba[o + 1] = g;
            rgba[o + 2] = b;
            rgba[o + 3] = a;
        }
    }
    mutableBitmap.UnlockBits(&data);

    if (terrainTex_ == 0) {
        glGenTextures(1, &terrainTex_);
    }
    glBindTexture(GL_TEXTURE_2D, terrainTex_);
    // Nearest filtering keeps the blocky pixel look.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
}

} // namespace net::minecraft::client::gl3d

#endif // _WIN32
