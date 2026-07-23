#pragma once
#include "net/minecraft/client/particle/Particle.hpp"
namespace net::minecraft::client::particle {
class ExplosionParticle : public Particle {
 public:
 ExplosionParticle(World* world, double x, double y, double z, double velocityX, double velocityY, double velocityZ)
     : Particle(world, x, y, z, velocityX, velocityY, velocityZ) {
  this->velocityX = velocityX + (random.nextFloat() * 2.0f - 1.0f) * 0.05f;
  this->velocityY = velocityY + (random.nextFloat() * 2.0f - 1.0f) * 0.05f;
  this->velocityZ = velocityZ + (random.nextFloat() * 2.0f - 1.0f) * 0.05f;
  red = green = blue = random.nextFloat() * 0.3f + 0.7f;
  scale = random.nextFloat() * random.nextFloat() * 6.0f + 1.0f;
  maxParticleAge = static_cast<int>(16.0f / (random.nextFloat() * 0.8f + 0.2f)) + 2;
 }
 void tick() override {
  Particle::tick();
  textureId = maxParticleAge > 0 ? 7 - particleAge * 8 / maxParticleAge : 0;
  velocityY += 0.004;
  velocityX *= 0.9 / 0.98;
  velocityY *= 0.9 / 0.98;
  velocityZ *= 0.9 / 0.98;
 }
};
} // namespace net::minecraft::client::particle
