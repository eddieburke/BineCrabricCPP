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
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            float sum = 0.0f;
            const int waveY = static_cast<int>(MathHelper::sin(static_cast<float>(y) * kPi * 2.0f / 16.0f) * 1.2f);
            const int waveX = static_cast<int>(MathHelper::sin(static_cast<float>(x) * kPi * 2.0f / 16.0f) * 1.2f);
            for (int nx = x - 1; nx <= x + 1; ++nx) {
                for (int ny = y - 1; ny <= y + 1; ++ny) {
                    const int sampleX = (nx + waveY) & 0xF;
                    const int sampleY = (ny + waveX) & 0xF;
                    sum += current[static_cast<std::size_t>(sampleX + sampleY * 16)];
                }
            }
            next[static_cast<std::size_t>(x + y * 16)] =
                sum / 10.0f +
                (heat[static_cast<std::size_t>(((x + 0) & 0xF) + ((y + 0) & 0xF) * 16)] +
                    heat[static_cast<std::size_t>(((x + 1) & 0xF) + ((y + 0) & 0xF) * 16)] +
                    heat[static_cast<std::size_t>(((x + 1) & 0xF) + ((y + 1) & 0xF) * 16)] +
                    heat[static_cast<std::size_t>(((x + 0) & 0xF) + ((y + 1) & 0xF) * 16)]) /
                    4.0f * 0.8f;
            integrateHeatCell(x + y * 16, 0.01f, 0.06f, 1.5f, 0.005);
        }
    }

    std::swap(next, current);

    for (int pixelIndex = 0; pixelIndex < 256; ++pixelIndex) {
        float brightness = current[static_cast<std::size_t>((pixelIndex - ticks / 3 * 16) & 0xFF)] * 2.0f;
        detail::clamp01(brightness);
        int red = static_cast<int>(brightness * 100.0f + 155.0f);
        int green = static_cast<int>(brightness * brightness * 255.0f);
        int blue = static_cast<int>(brightness * brightness * brightness * brightness * 128.0f);
        detail::writePixel(pixels, pixelIndex, red, green, blue, -1);
    }
}

} // namespace net::minecraft::client::render::texture
