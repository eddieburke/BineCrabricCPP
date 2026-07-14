#pragma once
#include "net/minecraft/client/particle/Particle.hpp"
namespace net::minecraft::client::particle {
class FireSmokeParticle : public Particle {
public:
  FireSmokeParticle(World* world,
                    double x,
                    double y,
                    double z,
                    double velocityX,
                    double velocityY,
                    double velocityZ,
                    float scaleIn = 1.0f)
      : Particle(world, x, y, z, 0.0, 0.0, 0.0) {
    this->velocityX = this->velocityX * 0.1 + velocityX;
    this->velocityY = this->velocityY * 0.1 + velocityY;
    this->velocityZ = this->velocityZ * 0.1 + velocityZ;
    red = green = blue = random.nextFloat() * 0.3f;
    scale *= 0.75f * scaleIn;
    initialScale = scale;
    maxParticleAge = static_cast<int>((8.0f / (random.nextFloat() * 0.8f + 0.2f)) * scaleIn);
    noClip = false;
  }
  void render(render::Tessellator& tessellator,
              float partialTicks,
              float horizontalSize,
              float verticalSize,
              float depthSize,
              float widthOffset,
              float heightOffset) override {
    float f = maxParticleAge > 0
                  ? (static_cast<float>(particleAge) + partialTicks) / static_cast<float>(maxParticleAge) * 32.0f
                  : 0.0f;
    f = std::clamp(f, 0.0f, 1.0f);
    scale = initialScale * f;
    Particle::render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
  }
  void tick() override {
    prevX = x;
    prevY = y;
    prevZ = z;
    if(particleAge++ >= maxParticleAge) {
      markDead();
    }
    textureId = maxParticleAge > 0 ? 7 - particleAge * 8 / maxParticleAge : 0;
    velocityY += 0.004;
    move(velocityX, velocityY, velocityZ);
    if(y == prevY) {
      velocityX *= 1.1;
      velocityZ *= 1.1;
    }
    velocityX *= 0.96;
    velocityY *= 0.96;
    velocityZ *= 0.96;
    if(onGround) {
      velocityX *= 0.7;
      velocityZ *= 0.7;
    }
  }
  float initialScale = 1.0f;
};
} // namespace net::minecraft::client::particle
