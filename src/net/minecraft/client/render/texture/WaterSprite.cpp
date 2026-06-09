#include "net/minecraft/client/render/texture/WaterSprite.hpp"

#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"

namespace net::minecraft::client::render::texture {

WaterSprite::WaterSprite()
    : LiquidSprite(detail::blockTextureId(8, 12 * 16 + 13))
{
}

void WaterSprite::tick()
{
    ++ticks;
    for (int n4 = 0; n4 < 16; ++n4) {
        for (int n3 = 0; n3 < 16; ++n3) {
            float f = 0.0f;
            for (int i = n4 - 1; i <= n4 + 1; ++i) {
                const int n2 = i & 0xF;
                const int n = n3 & 0xF;
                f += current[static_cast<std::size_t>(n2 + n * 16)];
            }
            next[static_cast<std::size_t>(n4 + n3 * 16)] = f / 3.3f + heat[static_cast<std::size_t>(n4 + n3 * 16)] * 0.8f;
        }
    }
    for (int n4 = 0; n4 < 16; ++n4) {
        for (int n3 = 0; n3 < 16; ++n3) {
            integrateHeatCell(n4 + n3 * 16, 0.05f, 0.1f, 0.5f, 0.05);
        }
    }

    std::swap(next, current);

    for (int n3 = 0; n3 < 256; ++n3) {
        float f = current[static_cast<std::size_t>(n3)];
        detail::clamp01(f);
        const float f2 = f * f;
        int n2 = static_cast<int>(32.0f + f2 * 32.0f);
        int n = static_cast<int>(50.0f + f2 * 64.0f);
        int n7 = 255;
        const int n8 = static_cast<int>(146.0f + f2 * 50.0f);
        detail::writePixel(pixels, n3, n2, n, n7, n8);
    }
}

} // namespace net::minecraft::client::render::texture
