#include "net/minecraft/client/render/texture/FireSprite.hpp"

#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"

namespace net::minecraft::client::render::texture {

FireSprite::FireSprite(int index)
    : DynamicTexture(detail::blockTextureId(51, 31) + index * 16)
{
}

void FireSprite::tick()
{
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 20; ++y) {
            int neighborCount = 18;
            float heatSum = current[static_cast<std::size_t>(x + (y + 1) % 20 * 16)] * static_cast<float>(neighborCount);
            for (int neighborX = x - 1; neighborX <= x + 1; ++neighborX) {
                for (int neighborY = y; neighborY <= y + 1; ++neighborY) {
                    if (neighborX >= 0 && neighborY >= 0 && neighborX < 16 && neighborY < 20) {
                        heatSum += current[static_cast<std::size_t>(neighborX + neighborY * 16)];
                    }
                    ++neighborCount;
                }
            }
            next[static_cast<std::size_t>(x + y * 16)] = heatSum / (static_cast<float>(neighborCount) * 1.06f);
            if (y < 19) {
                continue;
            }
            next[static_cast<std::size_t>(x + y * 16)] = static_cast<float>(
                detail::mathRandom() * detail::mathRandom() * detail::mathRandom() * 4.0 +
                detail::mathRandom() * 0.1 + 0.2);
        }
    }

    std::swap(next, current);

    for (int pixelIndex = 0; pixelIndex < 256; ++pixelIndex) {
        float intensity = current[static_cast<std::size_t>(pixelIndex)] * 1.8f;
        if (intensity > 1.0f) {
            intensity = 1.0f;
        }
        if (intensity < 0.0f) {
            intensity = 0.0f;
        }
        float heat = intensity;
        int red = static_cast<int>(heat * 155.0f + 100.0f);
        int green = static_cast<int>(heat * heat * 255.0f);
        int blue = static_cast<int>(heat * heat * heat * heat * heat * heat * heat * heat * heat * heat * 255.0f);
        int alpha = 255;
        if (heat < 0.5f) {
            alpha = 0;
        }
        heat = (heat - 0.5f) * 2.0f;
        detail::writePixel(pixels, pixelIndex, red, green, blue, alpha);
    }
}

} // namespace net::minecraft::client::render::texture
