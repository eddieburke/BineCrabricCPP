#pragma once
#include "net/minecraft/client/particle/RainSplashParticle.hpp"
namespace net::minecraft::client::particle {
class WaterSplashParticle : public RainSplashParticle {
 public:
 WaterSplashParticle(
     World* world, double x, double y, double z, double velocityX, double velocityY, double velocityZ)
     : RainSplashParticle(world, x, y, z) {
  gravityStrength = 0.04f;
  ++textureId;
  if(velocityY == 0.0 && (velocityX != 0.0 || velocityZ != 0.0)) {
   this->velocityX = velocityX;
   this->velocityY = velocityY + 0.1;
   this->velocityZ = velocityZ;
  }
 }
};
} // namespace net::minecraft::client::particle
