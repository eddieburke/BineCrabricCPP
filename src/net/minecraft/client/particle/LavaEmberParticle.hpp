#pragma once
#include "net/minecraft/client/particle/Particle.hpp"
namespace net::minecraft::client::particle {
class LavaEmberParticle : public Particle {
public:
  LavaEmberParticle(World* world, double x, double y, double z) : Particle(world, x, y, z, 0.0, 0.0, 0.0) {
    velocityX *= 0.8;
    velocityY = random.nextFloat() * 0.4f + 0.05f;
    velocityZ *= 0.8;
    red = green = blue = 1.0f;
    scale *= random.nextFloat() * 2.0f + 0.2f;
    initialScale = scale;
    maxParticleAge = static_cast<int>(16.0f / (random.nextFloat() * 0.8f + 0.2f));
    textureId = 49;
  }
  float getBrightnessAtEyes(float /*tickDelta*/) const override {
    return 1.0f;
  }
  void render(render::Tessellator& tessellator,
              float partialTicks,
              float horizontalSize,
              float verticalSize,
              float depthSize,
              float widthOffset,
              float heightOffset) override {
    const float f = maxParticleAge > 0
                        ? (static_cast<float>(particleAge) + partialTicks) / static_cast<float>(maxParticleAge)
                        : 1.0f;
    scale = initialScale * (1.0f - f * f);
    Particle::render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
  }
  void tick() override {
    prevX = x;
    prevY = y;
    prevZ = z;
    if(particleAge++ >= maxParticleAge) {
      markDead();
    }
    const float progress =
        maxParticleAge > 0 ? static_cast<float>(particleAge) / static_cast<float>(maxParticleAge) : 1.0f;
    if(random.nextFloat() > progress && world != nullptr) {
      world->addParticle("smoke", x, y, z, velocityX, velocityY, velocityZ);
    }
    velocityY -= 0.03;
    move(velocityX, velocityY, velocityZ);
    velocityX *= 0.999;
    velocityY *= 0.999;
    velocityZ *= 0.999;
    if(onGround) {
      velocityX *= 0.7;
      velocityZ *= 0.7;
    }
  }
  float initialScale = 1.0f;
};
} // namespace net::minecraft::client::particle
