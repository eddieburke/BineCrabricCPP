#include "net/minecraft/client/render/texture/WaterSideSprite.hpp"

#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"

namespace net::minecraft::client::render::texture {
WaterSideSprite::WaterSideSprite() : LiquidSprite(detail::blockTextureId(8, 12 * 16 + 13) + 1) {
    replicate = 2;
}

void WaterSideSprite::tick() {
    ++ticks;
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            float sum = 0.0f;
            for (int i = y - 2; i <= y; ++i) {
                const int nx = x & 0xF;
                const int ny = i & 0xF;
                sum += current[static_cast<std::size_t>(nx + ny * 16)];
            }
            next[static_cast<std::size_t>(x + y * 16)] = sum / 3.2f + heat[static_cast<std::size_t>(x + y * 16)] * 0.8f;
        }
    }
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            integrateHeatCell(x + y * 16, 0.05f, 0.3f, 0.5f, 0.2);
        }
    }
    std::swap(next, current);
    for (int pixelIndex = 0; pixelIndex < 256; ++pixelIndex) {
        float brightness = current[static_cast<std::size_t>((pixelIndex - ticks * 16) & 0xFF)];
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
