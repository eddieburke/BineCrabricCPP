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
    for (int frameIndex = 0; frameIndex < 32; ++frameIndex) {
        for (int texU = 0; texU < 16; ++texU) {
            for (int texV = 0; texV < 16; ++texV) {
                float noise = 0.0f;
                for (int octave = 0; octave < 2; ++octave) {
                    const float octaveOffsetU = static_cast<float>(octave * 8);
                    const float octaveOffsetV = static_cast<float>(octave * 8);
                    float normU = (static_cast<float>(texU) - octaveOffsetU) / 16.0f * 2.0f;
                    float normV = (static_cast<float>(texV) - octaveOffsetV) / 16.0f * 2.0f;
                    if (normU < -1.0f) {
                        normU += 2.0f;
                    }
                    if (normU >= 1.0f) {
                        normU -= 2.0f;
                    }
                    if (normV < -1.0f) {
                        normV += 2.0f;
                    }
                    if (normV >= 1.0f) {
                        normV -= 2.0f;
                    }
                    const float radiusSq = normU * normU + normV * normV;
                    float swirl = std::atan2(static_cast<double>(normV), static_cast<double>(normU)) +
                        (static_cast<float>(frameIndex) / 32.0f * kPi * 2.0f - radiusSq * 10.0f +
                            static_cast<float>(octave * 2)) *
                            static_cast<float>(octave * 2 - 1);
                    swirl = (MathHelper::sin(swirl) + 1.0f) / 2.0f;
                    swirl /= radiusSq + 1.0f;
                    noise += swirl * 0.5f;
                }
                noise += random.nextFloat() * 0.1f;
                const int blue = static_cast<int>(noise * 100.0f + 155.0f);
                const int green = static_cast<int>(noise * noise * 200.0f + 55.0f);
                const int red = static_cast<int>(noise * noise * noise * noise * 255.0f);
                const int alpha = static_cast<int>(noise * 100.0f + 155.0f);
                const int pixelIndex = texV * 16 + texU;
                frames[static_cast<std::size_t>(frameIndex)][static_cast<std::size_t>(pixelIndex) * 4 + 0] = static_cast<std::uint8_t>(green);
                frames[static_cast<std::size_t>(frameIndex)][static_cast<std::size_t>(pixelIndex) * 4 + 1] = static_cast<std::uint8_t>(red);
                frames[static_cast<std::size_t>(frameIndex)][static_cast<std::size_t>(pixelIndex) * 4 + 2] = static_cast<std::uint8_t>(blue);
                frames[static_cast<std::size_t>(frameIndex)][static_cast<std::size_t>(pixelIndex) * 4 + 3] = static_cast<std::uint8_t>(alpha);
            }
        }
    }
}

void NetherPortalSprite::tick()
{
    ++ticks;
    const std::array<std::uint8_t, 1024>& frame = frames[static_cast<std::size_t>(ticks & 0x1F)];
    for (int pixelIndex = 0; pixelIndex < 256; ++pixelIndex) {
        const int green = frame[static_cast<std::size_t>(pixelIndex) * 4 + 0] & 0xFF;
        const int red = frame[static_cast<std::size_t>(pixelIndex) * 4 + 1] & 0xFF;
        const int blue = frame[static_cast<std::size_t>(pixelIndex) * 4 + 2] & 0xFF;
        const int alpha = frame[static_cast<std::size_t>(pixelIndex) * 4 + 3] & 0xFF;
        detail::writePixel(pixels, pixelIndex, green, red, blue, alpha);
    }
}

} // namespace net::minecraft::client::render::texture
