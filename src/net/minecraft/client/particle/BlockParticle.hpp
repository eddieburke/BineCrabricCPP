#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
namespace net::minecraft::client::particle {
class BlockParticle : public Particle {
public:
  BlockParticle(World* world,
                double x,
                double y,
                double z,
                double velocityX,
                double velocityY,
                double velocityZ,
                Block* block,
                int side,
                int meta)
      : Particle(world, x, y, z, velocityX, velocityY, velocityZ), block_(block), side_(side) {
    if(block_ != nullptr) {
      textureId = block_->getTexture(0, meta);
      gravityStrength = block_->particleFallSpeedModifier;
    }
    red = green = blue = 0.6f;
    scale *= 0.5f;
  }
  BlockParticle* color(int x, int y, int z) {
    if(block_ == nullptr || block_ == Block::GRASS_BLOCK || world == nullptr) {
      return this;
    }
    const int color = block_->getColorMultiplier(world, x, y, z);
    red *= static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    green *= static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    blue *= static_cast<float>(color & 0xFF) / 255.0f;
    return this;
  }
  [[nodiscard]] int getGroup() const override {
    return 1;
  }
  [[nodiscard]] int boundTextureGl(texture::TextureManager& textureManager) const override {
    if(!mod::isMod(textureId)) {
      return -1;
    }
    return mod::glId(textureManager, textureId);
  }
  void render(render::Tessellator& tessellator,
              float partialTicks,
              float horizontalSize,
              float verticalSize,
              float depthSize,
              float widthOffset,
              float heightOffset) override {
    float u0 = 0.0f;
    float u1 = 0.0f;
    float v0 = 0.0f;
    float v1 = 0.0f;
    if(mod::isMod(textureId)) {
      const mod::TileScale tile = mod::tileScale(textureId);
      const float inv = static_cast<float>(tile.inv);
      // Match vanilla BlockParticle sampling: 4x4 sub-tiles inside a 16x16 tile.
      u0 = (static_cast<float>(tile.u) + prevU * 4.0f) * inv;
      u1 = u0 + 3.99f * inv;
      v0 = (static_cast<float>(tile.v) + prevV * 4.0f) * inv;
      v1 = v0 + 3.99f * inv;
    } else {
      u0 = (static_cast<float>(textureId % 16) + prevU / 4.0f) / 16.0f;
      u1 = u0 + 0.015609375f;
      v0 = (static_cast<float>(textureId / 16) + prevV / 4.0f) / 16.0f;
      v1 = v0 + 0.015609375f;
    }
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

private:
  Block* block_ = nullptr;
  int side_ = 0;
};
} // namespace net::minecraft::client::particle
