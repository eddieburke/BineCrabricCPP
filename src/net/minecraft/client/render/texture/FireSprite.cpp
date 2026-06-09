#include "net/minecraft/client/render/texture/FireSprite.hpp"

#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"

namespace net::minecraft::client::render::texture {

FireSprite::FireSprite(int index)
    : DynamicTexture(detail::blockTextureId(51, 31) + index * 16)
{
}

void FireSprite::tick()
{
    for (int i = 0; i < 16; ++i) {
        for (int n5 = 0; n5 < 20; ++n5) {
            int n6 = 18;
            float f = current[static_cast<std::size_t>(i + (n5 + 1) % 20 * 16)] * static_cast<float>(n6);
            for (int n4 = i - 1; n4 <= i + 1; ++n4) {
                for (int n3 = n5; n3 <= n5 + 1; ++n3) {
                    const int n2 = n4;
                    const int n = n3;
                    if (n2 >= 0 && n >= 0 && n2 < 16 && n < 20) {
                        f += current[static_cast<std::size_t>(n2 + n * 16)];
                    }
                    ++n6;
                }
            }
            next[static_cast<std::size_t>(i + n5 * 16)] = f / (static_cast<float>(n6) * 1.06f);
            if (n5 < 19) {
                continue;
            }
            next[static_cast<std::size_t>(i + n5 * 16)] = static_cast<float>(
                detail::mathRandom() * detail::mathRandom() * detail::mathRandom() * 4.0 +
                detail::mathRandom() * 0.1 + 0.2);
        }
    }

    std::swap(next, current);

    for (int n5 = 0; n5 < 256; ++n5) {
        float f2 = current[static_cast<std::size_t>(n5)] * 1.8f;
        if (f2 > 1.0f) {
            f2 = 1.0f;
        }
        if (f2 < 0.0f) {
            f2 = 0.0f;
        }
        float f = f2;
        int n4 = static_cast<int>(f * 155.0f + 100.0f);
        int n3 = static_cast<int>(f * f * 255.0f);
        int n2 = static_cast<int>(f * f * f * f * f * f * f * f * f * f * 255.0f);
        int n = 255;
        if (f < 0.5f) {
            n = 0;
        }
        f = (f - 0.5f) * 2.0f;
        detail::writePixel(pixels, n5, n4, n3, n2, n);
    }
}

} // namespace net::minecraft::client::render::texture
