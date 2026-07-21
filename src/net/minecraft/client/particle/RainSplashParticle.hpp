#pragma once
#include "net/minecraft/block/LiquidBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::particle {
class RainSplashParticle : public Particle {
 public:
 RainSplashParticle(World* world, double x, double y, double z) : Particle(world, x, y, z, 0.0, 0.0, 0.0) {
  velocityX *= 0.3;
  velocityY = random.nextFloat() * 0.2f + 0.1f;
  velocityZ *= 0.3;
  red = green = blue = 1.0f;
  textureId = 19 + random.nextInt(4);
  setBoundingBoxSpacing(0.01f, 0.01f);
  gravityStrength = 0.06f;
  maxParticleAge = static_cast<int>(8.0f / (random.nextFloat() * 0.8f + 0.2f));
 }
 void tick() override {
  prevX = x;
  prevY = y;
  prevZ = z;
  velocityY -= static_cast<double>(gravityStrength);
  move(velocityX, velocityY, velocityZ);
  velocityX *= 0.98;
  velocityY *= 0.98;
  velocityZ *= 0.98;
  if(maxParticleAge-- <= 0) {
   markDead();
  }
  if(onGround) {
   if(random.nextFloat() < 0.5f) {
    markDead();
   }
   velocityX *= 0.7;
   velocityZ *= 0.7;
  }
  if(world == nullptr) {
   return;
  }
  const block::material::Material& material =
      world->getMaterial(MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z));
  if((material.isFluid() || material.isSolid()) &&
     y < static_cast<double>(static_cast<float>(MathHelper::floor(y) + 1) -
                             block::LiquidBlock::getFluidHeightFromMeta(world->getBlockMeta(
                                 MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z))))) {
   markDead();
  }
 }
};
} // namespace net::minecraft::client::particle
