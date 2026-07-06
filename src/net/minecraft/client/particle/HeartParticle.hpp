#pragma once
#include "net/minecraft/client/particle/Particle.hpp"
namespace net::minecraft::client::particle {
class HeartParticle : public Particle {
public:
  HeartParticle(World* world, double x, double y, double z, double velocityX, double velocityY, double velocityZ,
                float scaleIn = 2.0f)
      : Particle(world, x, y, z, 0.0, 0.0, 0.0) {
    (void)velocityX;
    (void)velocityY;
    (void)velocityZ;
    velocityX *= 0.01;
    velocityY = velocityY * 0.01 + 0.1;
    velocityZ *= 0.01;
    scale *= 0.75f * scaleIn;
    initialScale = scale;
    maxParticleAge = 16;
    textureId = 80;
  }
  void render(render::Tessellator& tessellator, float partialTicks, float horizontalSize, float verticalSize,
              float depthSize, float widthOffset, float heightOffset) override {
    float f = maxParticleAge > 0
                  ? (static_cast<float>(particleAge) + partialTicks) / static_cast<float>(maxParticleAge) * 32.0f
                  : 0.0f;
    scale = initialScale * std::clamp(f, 0.0f, 1.0f);
    Particle::render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
  }
  float initialScale = 1.0f;
};
} // namespace net::minecraft::client::particle
