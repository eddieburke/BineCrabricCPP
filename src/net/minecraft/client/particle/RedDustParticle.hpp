#pragma once
#include "net/minecraft/client/particle/Particle.hpp"
namespace net::minecraft::client::particle {
class RedDustParticle : public Particle {
 public:
 RedDustParticle(World* world, double x, double y, double z, float velocityX, float velocityY, float velocityZ)
     : RedDustParticle(world, x, y, z, 1.0f, velocityX, velocityY, velocityZ) {
 }
 RedDustParticle(World* world, double x, double y, double z, float scaleIn, float redIn, float greenIn, float blueIn)
     : Particle(world, x, y, z, 0.0, 0.0, 0.0) {
  velocityX *= 0.1;
  velocityY *= 0.1;
  velocityZ *= 0.1;
  if(redIn == 0.0f) {
   redIn = 1.0f;
  }
  const float brightness = random.nextFloat() * 0.4f + 0.6f;
  red = (random.nextFloat() * 0.2f + 0.8f) * redIn * brightness;
  green = (random.nextFloat() * 0.2f + 0.8f) * greenIn * brightness;
  blue = (random.nextFloat() * 0.2f + 0.8f) * blueIn * brightness;
  scale *= 0.75f * scaleIn;
  startScale = scale;
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
  scale = startScale * f;
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
 float startScale = 1.0f;
};
} // namespace net::minecraft::client::particle
