#include "net/minecraft/client/render/texture/CompassSprite.hpp"

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

CompassSprite::CompassSprite(Minecraft& minecraft)
    : DynamicTexture(
          Item::ITEMS[345] != nullptr ? Item::ITEMS[345]->getTextureId(0) : (6 + 3 * 16)),
      minecraft_(minecraft)
{
    atlas = 1;
    const ::net::minecraft::client::texture::RasterImage items =
        ::net::minecraft::client::texture::TextureManager::loadRasterFromFile(
            ::net::minecraft::client::texture::TextureManager::resolveResourcePath("gui/items.png"));
    if (items.width > 0 && items.height > 0) {
        detail::copySpriteArgb(items.argb, items.width, sprite, compass);
    }
}

void CompassSprite::tick()
{
    for (int i = 0; i < 256; ++i) {
        int n12 = static_cast<int>((compass[static_cast<std::size_t>(i)] >> 24) & 0xFFU);
        int n13 = static_cast<int>((compass[static_cast<std::size_t>(i)] >> 16) & 0xFFU);
        int n14 = static_cast<int>((compass[static_cast<std::size_t>(i)] >> 8) & 0xFFU);
        int n15 = static_cast<int>(compass[static_cast<std::size_t>(i)] & 0xFFU);
        detail::writePixel(pixels, i, n13, n14, n15, n12);
    }

    double d2 = 0.0;
    World* world = minecraft_.world;
    Entity* player = minecraft_.player;
    if (world != nullptr && player != nullptr) {
        const Vec3i spawn = world->getSpawnPos();
        const double d3 = static_cast<double>(spawn.x) - player->x;
        const double d4 = static_cast<double>(spawn.z) - player->z;
        d2 = static_cast<double>(player->yaw - 90.0f) * kPi / 180.0 -
            std::atan2(d4, d3);
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
    const double d5 = std::sin(angle);
    const double d6 = std::cos(angle);

    for (int n11 = -4; n11 <= 4; ++n11) {
        const int n10 = static_cast<int>(8.5 + d6 * static_cast<double>(n11) * 0.3);
        const int n9 = static_cast<int>(7.5 - d5 * static_cast<double>(n11) * 0.3 * 0.5);
        const int n8 = n9 * 16 + n10;
        int n7 = 100;
        int n6c = 100;
        int n5 = 100;
        const int n4 = 255;
        detail::writePixel(pixels, n8, n7, n6c, n5, n4);
    }

    for (int n11 = -8; n11 <= 16; ++n11) {
        const int n10 = static_cast<int>(8.5 + d5 * static_cast<double>(n11) * 0.3);
        const int n9 = static_cast<int>(7.5 + d6 * static_cast<double>(n11) * 0.3 * 0.5);
        const int n8 = n9 * 16 + n10;
        int n7 = n11 >= 0 ? 255 : 100;
        int n6c = n11 >= 0 ? 20 : 100;
        int n5 = n11 >= 0 ? 20 : 100;
        const int n4 = 255;
        detail::writePixel(pixels, n8, n7, n6c, n5, n4);
    }
}

} // namespace net::minecraft::client::render::texture
