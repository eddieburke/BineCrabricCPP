#include "net/minecraft/client/render/texture/ClockSprite.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/world/World.hpp"

#include <cmath>

namespace net::minecraft::client::render::texture {

namespace {
constexpr double kPi = 3.1415927410125732;
}

ClockSprite::ClockSprite(Minecraft& minecraft)
    : DynamicTexture(Item::ITEMS[347] != nullptr ? Item::ITEMS[347]->getTextureId(0) : (6 + 4 * 16)),
      minecraft_(minecraft)
{
    atlas = 1;
    const ::net::minecraft::client::texture::RasterImage items =
        ::net::minecraft::client::texture::TextureManager::loadRasterFromFile(
            ::net::minecraft::client::texture::TextureManager::resolveResourcePath("gui/items.png"));
    if (items.width > 0 && items.height > 0) {
        detail::copySpriteArgb(items.argb, items.width, sprite, clock);
    }
    const ::net::minecraft::client::texture::RasterImage dialImage =
        ::net::minecraft::client::texture::TextureManager::loadRasterFromFile(
            ::net::minecraft::client::texture::TextureManager::resolveResourcePath("misc/dial.png"));
    if (dialImage.width >= 16 && dialImage.height >= 16) {
        detail::copySpriteArgb(dialImage.argb, dialImage.width, 0, dial);
    }
}

void ClockSprite::tick()
{
    double d2 = 0.0;
    World* world = minecraft_.world;
    Entity* player = minecraft_.player;
    if (world != nullptr && player != nullptr) {
        const float f = world->getTime(1.0f);
        d2 = -static_cast<double>(f) * kPi * 2.0;
        if (world->dimension != nullptr && world->dimension->isNether) {
            d2 = detail::mathRandom() * kPi * 2.0;
        }
    }

    double d = d2 - angle;
    for (; d < -kPi; d += kPi * 2.0) {
    }
    while (d >= kPi) {
        d -= kPi * 2.0;
    }
    if (d < -1.0) {
        d = -1.0;
    }
    if (d > 1.0) {
        d = 1.0;
    }
    angleDelta += d * 0.1;
    angleDelta *= 0.8;
    angle += angleDelta;
    const double d3 = std::sin(angle);
    const double d4 = std::cos(angle);

    for (int i = 0; i < 256; ++i) {
        int n = static_cast<int>((clock[static_cast<std::size_t>(i)] >> 24) & 0xFFU);
        int n2 = static_cast<int>((clock[static_cast<std::size_t>(i)] >> 16) & 0xFFU);
        int n3 = static_cast<int>((clock[static_cast<std::size_t>(i)] >> 8) & 0xFFU);
        int n4 = static_cast<int>(clock[static_cast<std::size_t>(i)] & 0xFFU);
        if (n2 == n4 && n3 == 0 && n4 > 0) {
            const double d5 = -((static_cast<double>(i % 16) / 15.0) - 0.5);
            const double d6 = static_cast<double>(i / 16) / 15.0 - 0.5;
            const int n5 = n2;
            const int n6 = static_cast<int>((d5 * d4 + d6 * d3 + 0.5) * 16.0);
            const int n7 = static_cast<int>((d6 * d4 - d5 * d3 + 0.5) * 16.0);
            const int n8 = (n6 & 0xF) + (n7 & 0xF) * 16;
            n = static_cast<int>((dial[static_cast<std::size_t>(n8)] >> 24) & 0xFFU);
            n2 = (static_cast<int>((dial[static_cast<std::size_t>(n8)] >> 16) & 0xFFU) * n5) / 255;
            n3 = (static_cast<int>((dial[static_cast<std::size_t>(n8)] >> 8) & 0xFFU) * n5) / 255;
            n4 = (static_cast<int>(dial[static_cast<std::size_t>(n8)] & 0xFFU) * n5) / 255;
        }
        detail::writePixel(pixels, i, n2, n3, n4, n);
    }
}

} // namespace net::minecraft::client::render::texture
