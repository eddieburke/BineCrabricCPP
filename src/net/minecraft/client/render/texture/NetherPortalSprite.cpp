#include "net/minecraft/client/render/texture/NetherPortalSprite.hpp"

#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <cmath>

namespace net::minecraft::client::render::texture {

namespace {
constexpr float kPi = 3.1415927410125732f;
}

NetherPortalSprite::NetherPortalSprite()
    : DynamicTexture(detail::blockTextureId(90, 14))
{
    JavaRandom random(100ULL);
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 16; ++j) {
            for (int k = 0; k < 16; ++k) {
                float f = 0.0f;
                for (int n = 0; n < 2; ++n) {
                    const float f2 = static_cast<float>(n * 8);
                    const float f3 = static_cast<float>(n * 8);
                    float f4 = (static_cast<float>(j) - f2) / 16.0f * 2.0f;
                    float f5 = (static_cast<float>(k) - f3) / 16.0f * 2.0f;
                    if (f4 < -1.0f) {
                        f4 += 2.0f;
                    }
                    if (f4 >= 1.0f) {
                        f4 -= 2.0f;
                    }
                    if (f5 < -1.0f) {
                        f5 += 2.0f;
                    }
                    if (f5 >= 1.0f) {
                        f5 -= 2.0f;
                    }
                    const float f6 = f4 * f4 + f5 * f5;
                    float f7 = std::atan2(static_cast<double>(f5), static_cast<double>(f4)) +
                        (static_cast<float>(i) / 32.0f * kPi * 2.0f - f6 * 10.0f +
                            static_cast<float>(n * 2)) *
                            static_cast<float>(n * 2 - 1);
                    f7 = (MathHelper::sin(f7) + 1.0f) / 2.0f;
                    f7 /= f6 + 1.0f;
                    f += f7 * 0.5f;
                }
                f += random.nextFloat() * 0.1f;
                const int n = static_cast<int>(f * 100.0f + 155.0f);
                const int n2 = static_cast<int>(f * f * 200.0f + 55.0f);
                const int n3 = static_cast<int>(f * f * f * f * 255.0f);
                const int n4 = static_cast<int>(f * 100.0f + 155.0f);
                const int n5 = k * 16 + j;
                frames[static_cast<std::size_t>(i)][static_cast<std::size_t>(n5) * 4 + 0] = static_cast<std::uint8_t>(n2);
                frames[static_cast<std::size_t>(i)][static_cast<std::size_t>(n5) * 4 + 1] = static_cast<std::uint8_t>(n3);
                frames[static_cast<std::size_t>(i)][static_cast<std::size_t>(n5) * 4 + 2] = static_cast<std::uint8_t>(n);
                frames[static_cast<std::size_t>(i)][static_cast<std::size_t>(n5) * 4 + 3] = static_cast<std::uint8_t>(n4);
            }
        }
    }
}

void NetherPortalSprite::tick()
{
    ++ticks;
    const std::array<std::uint8_t, 1024>& frame = frames[static_cast<std::size_t>(ticks & 0x1F)];
    for (int i = 0; i < 256; ++i) {
        int n = frame[static_cast<std::size_t>(i) * 4 + 0] & 0xFF;
        int n2 = frame[static_cast<std::size_t>(i) * 4 + 1] & 0xFF;
        int n3 = frame[static_cast<std::size_t>(i) * 4 + 2] & 0xFF;
        int n4 = frame[static_cast<std::size_t>(i) * 4 + 3] & 0xFF;
        detail::writePixel(pixels, i, n, n2, n3, n4);
    }
}

} // namespace net::minecraft::client::render::texture
