#include "net/minecraft/client/render/texture/LavaSideSprite.hpp"

#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <cmath>

namespace net::minecraft::client::render::texture {

namespace {
constexpr float kPi = 3.1415927410125732f;
}

LavaSideSprite::LavaSideSprite()
    : LiquidSprite(detail::blockTextureId(10, 14 * 16 + 13) + 1)
{
    replicate = 2;
}

void LavaSideSprite::tick()
{
    ++ticks;
    for (int i = 0; i < 16; ++i) {
        for (int n6 = 0; n6 < 16; ++n6) {
            float f = 0.0f;
            const int n7 = static_cast<int>(MathHelper::sin(static_cast<float>(n6) * kPi * 2.0f / 16.0f) * 1.2f);
            const int n5 = static_cast<int>(MathHelper::sin(static_cast<float>(i) * kPi * 2.0f / 16.0f) * 1.2f);
            for (int n4 = i - 1; n4 <= i + 1; ++n4) {
                for (int n3 = n6 - 1; n3 <= n6 + 1; ++n3) {
                    const int n2 = (n4 + n7) & 0xF;
                    const int n = (n3 + n5) & 0xF;
                    f += current[static_cast<std::size_t>(n2 + n * 16)];
                }
            }
            next[static_cast<std::size_t>(i + n6 * 16)] =
                f / 10.0f +
                (heat[static_cast<std::size_t>(((i + 0) & 0xF) + ((n6 + 0) & 0xF) * 16)] +
                    heat[static_cast<std::size_t>(((i + 1) & 0xF) + ((n6 + 0) & 0xF) * 16)] +
                    heat[static_cast<std::size_t>(((i + 1) & 0xF) + ((n6 + 1) & 0xF) * 16)] +
                    heat[static_cast<std::size_t>(((i + 0) & 0xF) + ((n6 + 1) & 0xF) * 16)]) /
                    4.0f * 0.8f;
            integrateHeatCell(i + n6 * 16, 0.01f, 0.06f, 1.5f, 0.005);
        }
    }

    std::swap(next, current);

    for (int n6 = 0; n6 < 256; ++n6) {
        float f = current[static_cast<std::size_t>((n6 - ticks / 3 * 16) & 0xFF)] * 2.0f;
        detail::clamp01(f);
        const float f2 = f;
        int n5 = static_cast<int>(f2 * 100.0f + 155.0f);
        int n4 = static_cast<int>(f2 * f2 * 255.0f);
        int n3 = static_cast<int>(f2 * f2 * f2 * f2 * 128.0f);
        detail::writePixel(pixels, n6, n5, n4, n3, -1);
    }
}

} // namespace net::minecraft::client::render::texture
