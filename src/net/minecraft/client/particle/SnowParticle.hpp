#pragma once
#include "net/minecraft/client/particle/Particle.hpp"

namespace net::minecraft::client::particle {
class SnowParticle : public Particle {
   public:
    SnowParticle(World* world,
                 double x,
                 double y,
                 double z,
                 double velocityX,
                 double velocityY,
                 double velocityZ,
                 float scaleIn = 1.0f)
        : Particle(world, x, y, z, velocityX, velocityY, velocityZ) {
        this->velocityX = this->velocityX * 0.1 + velocityX;
        this->velocityY = this->velocityY * 0.1 + velocityY;
        this->velocityZ = this->velocityZ * 0.1 + velocityZ;
        red = green = blue = 1.0f - random.nextFloat() * 0.3f;
        scale *= 0.75f * scaleIn;
        initialScale = scale;
        maxParticleAge = static_cast<int>((8.0f / (random.nextFloat() * 0.8f + 0.2f)) * scaleIn);
    }

    void tick() override {
        Particle::tick();
        textureId = maxParticleAge > 0 ? 7 - particleAge * 8 / maxParticleAge : 0;
        velocityY -= 0.03;
    }

    float initialScale = 1.0f;
};
}  // namespace net::minecraft::client::particle
