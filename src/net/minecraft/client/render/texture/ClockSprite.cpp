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
    double targetAngle = 0.0;
    World* world = minecraft_.world;
    Entity* player = minecraft_.player;
    if (world != nullptr && player != nullptr) {
        const float dayTime = world->getTime(1.0f);
        targetAngle = -static_cast<double>(dayTime) * kPi * 2.0;
        if (world->dimension != nullptr && world->dimension->isNether) {
            targetAngle = detail::mathRandom() * kPi * 2.0;
        }
    }

    double angleError = targetAngle - angle;
    for (; angleError < -kPi; angleError += kPi * 2.0) {
    }
    while (angleError >= kPi) {
        angleError -= kPi * 2.0;
    }
    if (angleError < -1.0) {
        angleError = -1.0;
    }
    if (angleError > 1.0) {
        angleError = 1.0;
    }
    angleDelta += angleError * 0.1;
    angleDelta *= 0.8;
    angle += angleDelta;
    const double sinAngle = std::sin(angle);
    const double cosAngle = std::cos(angle);

    for (int i = 0; i < 256; ++i) {
        int alpha = static_cast<int>((clock[static_cast<std::size_t>(i)] >> 24) & 0xFFU);
        int red = static_cast<int>((clock[static_cast<std::size_t>(i)] >> 16) & 0xFFU);
        int green = static_cast<int>((clock[static_cast<std::size_t>(i)] >> 8) & 0xFFU);
        int blue = static_cast<int>(clock[static_cast<std::size_t>(i)] & 0xFFU);
        if (red == blue && green == 0 && blue > 0) {
            const double normX = -((static_cast<double>(i % 16) / 15.0) - 0.5);
            const double normY = static_cast<double>(i / 16) / 15.0 - 0.5;
            const int brightness = red;
            const int dialX = static_cast<int>((normX * cosAngle + normY * sinAngle + 0.5) * 16.0);
            const int dialY = static_cast<int>((normY * cosAngle - normX * sinAngle + 0.5) * 16.0);
            const int dialIndex = (dialX & 0xF) + (dialY & 0xF) * 16;
            alpha = static_cast<int>((dial[static_cast<std::size_t>(dialIndex)] >> 24) & 0xFFU);
            red = (static_cast<int>((dial[static_cast<std::size_t>(dialIndex)] >> 16) & 0xFFU) * brightness) / 255;
            green = (static_cast<int>((dial[static_cast<std::size_t>(dialIndex)] >> 8) & 0xFFU) * brightness) / 255;
            blue = (static_cast<int>(dial[static_cast<std::size_t>(dialIndex)] & 0xFFU) * brightness) / 255;
        }
        detail::writePixel(pixels, i, red, green, blue, alpha);
    }
}

} // namespace net::minecraft::client::render::texture
