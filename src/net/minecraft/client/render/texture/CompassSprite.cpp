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
    : DynamicTexture(Item::ITEMS[345] != nullptr ? Item::ITEMS[345]->getTextureId(0) : (6 + 3 * 16)),
      minecraft_(minecraft) {
  atlas = 1;
  const ::net::minecraft::client::texture::RasterImage items =
      ::net::minecraft::client::texture::TextureManager::loadRasterFromFile(
          ::net::minecraft::client::texture::TextureManager::resolveResourcePath("gui/items.png"));
  if(items.width > 0 && items.height > 0) {
    detail::copySpriteArgb(items.argb, items.width, sprite, compass);
  }
}
void CompassSprite::tick() {
  for(int i = 0; i < 256; ++i) {
    const int alpha = static_cast<int>((compass[static_cast<std::size_t>(i)] >> 24) & 0xFFU);
    const int red = static_cast<int>((compass[static_cast<std::size_t>(i)] >> 16) & 0xFFU);
    const int green = static_cast<int>((compass[static_cast<std::size_t>(i)] >> 8) & 0xFFU);
    const int blue = static_cast<int>(compass[static_cast<std::size_t>(i)] & 0xFFU);
    detail::writePixel(pixels, i, red, green, blue, alpha);
  }
  double targetAngle = 0.0;
  World* world = minecraft_.world;
  Entity* player = minecraft_.player;
  if(world != nullptr && player != nullptr) {
    const Vec3i spawn = world->getSpawnPos();
    const double toSpawnX = static_cast<double>(spawn.x) - player->x;
    const double toSpawnZ = static_cast<double>(spawn.z) - player->z;
    targetAngle = static_cast<double>(player->yaw - 90.0f) * kPi / 180.0 - std::atan2(toSpawnZ, toSpawnX);
    if(world->dimension != nullptr && world->dimension->isNether) {
      targetAngle = detail::mathRandom() * kPi * 2.0;
    }
  }
  double angleError = targetAngle - angle;
  for(; angleError < -kPi; angleError += kPi * 2.0) {
  }
  while(angleError >= kPi) {
    angleError -= kPi * 2.0;
  }
  if(angleError < -1.0) {
    angleError = -1.0;
  }
  if(angleError > 1.0) {
    angleError = 1.0;
  }
  angleDelta += angleError * 0.1;
  angleDelta *= 0.8;
  angle += angleDelta;
  const double sinAngle = std::sin(angle);
  const double cosAngle = std::cos(angle);
  for(int offset = -4; offset <= 4; ++offset) {
    const int pixelX = static_cast<int>(8.5 + cosAngle * static_cast<double>(offset) * 0.3);
    const int pixelY = static_cast<int>(7.5 - sinAngle * static_cast<double>(offset) * 0.3 * 0.5);
    const int pixelIndex = pixelY * 16 + pixelX;
    constexpr int red = 100;
    constexpr int green = 100;
    constexpr int blue = 100;
    constexpr int alpha = 255;
    detail::writePixel(pixels, pixelIndex, red, green, blue, alpha);
  }
  for(int offset = -8; offset <= 16; ++offset) {
    const int pixelX = static_cast<int>(8.5 + sinAngle * static_cast<double>(offset) * 0.3);
    const int pixelY = static_cast<int>(7.5 + cosAngle * static_cast<double>(offset) * 0.3 * 0.5);
    const int pixelIndex = pixelY * 16 + pixelX;
    const int red = offset >= 0 ? 255 : 100;
    const int green = offset >= 0 ? 20 : 100;
    const int blue = offset >= 0 ? 20 : 100;
    constexpr int alpha = 255;
    detail::writePixel(pixels, pixelIndex, red, green, blue, alpha);
  }
}
} // namespace net::minecraft::client::render::texture
