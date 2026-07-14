#pragma once
#include "net/minecraft/client/particle/Particle.hpp"
namespace net::minecraft::client::particle {
class PortalParticle : public Particle {
public:
  PortalParticle(World* world, double x, double y, double z, double velocityX, double velocityY, double velocityZ)
      : Particle(world, x, y, z, velocityX, velocityY, velocityZ), initialX(x), initialY(y), initialZ(z) {
    this->velocityX = velocityX;
    this->velocityY = velocityY;
    this->velocityZ = velocityZ;
    this->x = x;
    this->y = y;
    this->z = z;
    const float brightness = random.nextFloat() * 0.6f + 0.4f;
    initialScale = scale = random.nextFloat() * 0.2f + 0.5f;
    green = blue = brightness;
    red = blue;
    green *= 0.3f;
    red *= 0.9f;
    maxParticleAge = random.nextInt(10) + 40;
    noClip = true;
    textureId = random.nextInt(8);
  }
  void render(render::Tessellator& tessellator,
              float partialTicks,
              float horizontalSize,
              float verticalSize,
              float depthSize,
              float widthOffset,
              float heightOffset) override {
    float age = maxParticleAge > 0
                    ? (static_cast<float>(particleAge) + partialTicks) / static_cast<float>(maxParticleAge)
                    : 1.0f;
    age = 1.0f - age;
    age *= age;
    age = 1.0f - age;
    scale = initialScale * age;
    Particle::render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
  }
  [[nodiscard]] float getBrightnessAtEyes(float tickDelta) const override {
    const float base = Particle::getBrightnessAtEyes(tickDelta);
    float age = maxParticleAge > 0 ? static_cast<float>(particleAge) / static_cast<float>(maxParticleAge) : 1.0f;
    age *= age;
    age *= age;
    return base * (1.0f - age) + age;
  }
  void tick() override {
    prevX = x;
    prevY = y;
    prevZ = z;
    const float age =
        maxParticleAge > 0 ? static_cast<float>(particleAge) / static_cast<float>(maxParticleAge) : 1.0f;
    float curve = -age + age * age * 2.0f;
    curve = 1.0f - curve;
    x = initialX + velocityX * curve;
    y = initialY + velocityY * curve + (1.0f - age);
    z = initialZ + velocityZ * curve;
    if(particleAge++ >= maxParticleAge) {
      markDead();
    }
  }
  float initialScale = 1.0f;
  double initialX = 0.0;
  double initialY = 0.0;
  double initialZ = 0.0;
};
} // namespace net::minecraft::client::particle
