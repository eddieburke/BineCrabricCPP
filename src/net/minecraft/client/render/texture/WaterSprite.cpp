#include "net/minecraft/client/render/texture/WaterSprite.hpp"

#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"

namespace net::minecraft::client::render::texture {
WaterSprite::WaterSprite() : LiquidSprite(detail::blockTextureId(8, 12 * 16 + 13)) {
}

void WaterSprite::tick() {
    ++ticks;
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            float sum = 0.0f;
            for (int i = x - 1; i <= x + 1; ++i) {
                const int nx = i & 0xF;
                const int ny = y & 0xF;
                sum += current[static_cast<std::size_t>(nx + ny * 16)];
            }
            next[static_cast<std::size_t>(x + y * 16)] = sum / 3.3f + heat[static_cast<std::size_t>(x + y * 16)] * 0.8f;
        }
    }
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            integrateHeatCell(x + y * 16, 0.05f, 0.1f, 0.5f, 0.05);
        }
    }
    std::swap(next, current);
    for (int pixelIndex = 0; pixelIndex < 256; ++pixelIndex) {
        float brightness = current[static_cast<std::size_t>(pixelIndex)];
        detail::clamp01(brightness);
        const float intensity = brightness * brightness;
        int red = static_cast<int>(32.0f + intensity * 32.0f);
        int green = static_cast<int>(50.0f + intensity * 64.0f);
        int blue = 255;
        const int alpha = static_cast<int>(146.0f + intensity * 50.0f);
        detail::writePixel(pixels, pixelIndex, red, green, blue, alpha);
    }
}
}  // namespace net::minecraft::client::render::texture
