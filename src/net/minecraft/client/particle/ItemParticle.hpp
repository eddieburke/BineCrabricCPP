#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::client::particle {
class ItemParticle : public Particle {
 public:
 ItemParticle(World* world, double x, double y, double z, Item* item) : Particle(world, x, y, z, 0.0, 0.0, 0.0) {
  textureId = item != nullptr ? item->getTextureId(0) : 0;
  red = green = blue = 1.0f;
  gravityStrength = Block::SNOW_BLOCK != nullptr ? Block::SNOW_BLOCK->particleFallSpeedModifier : 1.0f;
  scale *= 0.5f;
 }
 [[nodiscard]] int getGroup() const override {
  return 2;
 }
 void render(render::Tessellator& tessellator,
             float partialTicks,
             float horizontalSize,
             float verticalSize,
             float depthSize,
             float widthOffset,
             float heightOffset) override {
  const float u0 = (static_cast<float>(textureId % 16) + prevU / 4.0f) / 16.0f;
  const float u1 = u0 + 0.015609375f;
  const float v0 = (static_cast<float>(textureId / 16) + prevV / 4.0f) / 16.0f;
  const float v1 = v0 + 0.015609375f;
  const float size = 0.1f * scale;
  const float px = static_cast<float>(prevX + (x - prevX) * partialTicks - xOffset);
  const float py = static_cast<float>(prevY + (y - prevY) * partialTicks - yOffset);
  const float pz = static_cast<float>(prevZ + (z - prevZ) * partialTicks - zOffset);
  const float brightness = getBrightnessAtEyes(partialTicks);
  tessellator.color(brightness * red, brightness * green, brightness * blue);
  tessellator.vertex(px - horizontalSize * size - widthOffset * size,
                     py - verticalSize * size,
                     pz - depthSize * size - heightOffset * size,
                     u0,
                     v1);
  tessellator.vertex(px - horizontalSize * size + widthOffset * size,
                     py + verticalSize * size,
                     pz - depthSize * size + heightOffset * size,
                     u0,
                     v0);
  tessellator.vertex(px + horizontalSize * size + widthOffset * size,
                     py + verticalSize * size,
                     pz + depthSize * size + heightOffset * size,
                     u1,
                     v0);
  tessellator.vertex(px + horizontalSize * size - widthOffset * size,
                     py - verticalSize * size,
                     pz + depthSize * size - heightOffset * size,
                     u1,
                     v1);
 }
};
} // namespace net::minecraft::client::particle
