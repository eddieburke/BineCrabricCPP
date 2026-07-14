#pragma once
#include <algorithm>
#include <cmath>
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::particle {
class Particle : public Entity {
public:
  inline static double xOffset = 0.0;
  inline static double yOffset = 0.0;
  inline static double zOffset = 0.0;
  Particle(World* world, double x, double y, double z, double velocityX, double velocityY, double velocityZ)
      : Entity(world) {
    setBoundingBoxSpacing(0.2f, 0.2f);
    standingEyeHeight = height / 2.0f;
    setPosition(x, y, z);
    blue = 1.0f;
    green = 1.0f;
    red = 1.0f;
    // Java uses Math.random() here; project convention maps it to the entity's
    // own random.nextDouble() (see TntEntity). nextDouble() draws twice, so the
    // earlier nextFloat() form desynced the whole downstream stream.
    this->velocityX = velocityX + static_cast<double>(static_cast<float>(random.nextDouble() * 2.0 - 1.0) * 0.4f);
    this->velocityY = velocityY + static_cast<double>(static_cast<float>(random.nextDouble() * 2.0 - 1.0) * 0.4f);
    this->velocityZ = velocityZ + static_cast<double>(static_cast<float>(random.nextDouble() * 2.0 - 1.0) * 0.4f);
    const float factor = static_cast<float>(random.nextDouble() + random.nextDouble() + 1.0) * 0.15f;
    const float speed = MathHelper::sqrt(this->velocityX * this->velocityX + this->velocityY * this->velocityY +
                                         this->velocityZ * this->velocityZ);
    this->velocityX = this->velocityX / speed * factor * 0.4f;
    this->velocityY = this->velocityY / speed * factor * 0.4f + 0.1f;
    this->velocityZ = this->velocityZ / speed * factor * 0.4f;
    prevU = random.nextFloat() * 3.0f;
    prevV = random.nextFloat() * 3.0f;
    scale = (random.nextFloat() * 0.5f + 0.5f) * 2.0f;
    maxParticleAge = static_cast<int>(4.0f / (random.nextFloat() * 0.9f + 0.1f));
    particleAge = 0;
  }
  Particle* multiplyVelocity(float factor) {
    velocityX *= factor;
    velocityY = (velocityY - 0.1) * factor + 0.1;
    velocityZ *= factor;
    return this;
  }
  Particle* setScale(float scaleIn) {
    setBoundingBoxSpacing(0.2f * scaleIn, 0.2f * scaleIn);
    scale *= scaleIn;
    return this;
  }
  void tick() override {
    prevX = x;
    prevY = y;
    prevZ = z;
    if(particleAge++ >= maxParticleAge) {
      markDead();
    }
    velocityY -= 0.04 * gravityStrength;
    move(velocityX, velocityY, velocityZ);
    velocityX *= 0.98;
    velocityY *= 0.98;
    velocityZ *= 0.98;
    if(onGround) {
      velocityX *= 0.7;
      velocityZ *= 0.7;
    }
  }
  virtual void render(render::Tessellator& tessellator,
                      float partialTicks,
                      float horizontalSize,
                      float verticalSize,
                      float depthSize,
                      float widthOffset,
                      float heightOffset) {
    const float u0 = static_cast<float>(textureId % 16) / 16.0f;
    const float u1 = u0 + 0.0624375f;
    const float v0 = static_cast<float>(textureId / 16) / 16.0f;
    const float v1 = v0 + 0.0624375f;
    const float size = 0.1f * scale;
    const float px = static_cast<float>(prevX + (x - prevX) * partialTicks - xOffset);
    const float py = static_cast<float>(prevY + (y - prevY) * partialTicks - yOffset);
    const float pz = static_cast<float>(prevZ + (z - prevZ) * partialTicks - zOffset);
    const float brightness = getBrightnessAtEyes(partialTicks);
    tessellator.color(red * brightness, green * brightness, blue * brightness);
    tessellator.vertex(px - horizontalSize * size - widthOffset * size,
                       py - verticalSize * size,
                       pz - depthSize * size - heightOffset * size,
                       u1,
                       v1);
    tessellator.vertex(px - horizontalSize * size + widthOffset * size,
                       py + verticalSize * size,
                       pz - depthSize * size + heightOffset * size,
                       u1,
                       v0);
    tessellator.vertex(px + horizontalSize * size + widthOffset * size,
                       py + verticalSize * size,
                       pz + depthSize * size + heightOffset * size,
                       u0,
                       v0);
    tessellator.vertex(px + horizontalSize * size - widthOffset * size,
                       py - verticalSize * size,
                       pz + depthSize * size - heightOffset * size,
                       u0,
                       v1);
  }
  [[nodiscard]] virtual int getGroup() const {
    return 0;
  }
  [[nodiscard]] virtual int boundTextureGl(texture::TextureManager& /*textureManager*/) const {
    return -1;
  }
  int textureId = 0;
  float prevU = 0.0f;
  float prevV = 0.0f;
  int particleAge = 0;
  int maxParticleAge = 0;
  float scale = 1.0f;
  float gravityStrength = 0.0f;
  float red = 1.0f;
  float green = 1.0f;
  float blue = 1.0f;
};
} // namespace net::minecraft::client::particle
