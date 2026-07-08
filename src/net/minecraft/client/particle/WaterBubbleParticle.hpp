#pragma once
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/particle/Particle.hpp"

namespace net::minecraft::client::particle {
class WaterBubbleParticle : public Particle {
   public:
    WaterBubbleParticle(
        World* world, double x, double y, double z, double velocityX, double velocityY, double velocityZ)
        : Particle(world, x, y, z, velocityX, velocityY, velocityZ) {
        red = green = blue = 1.0f;
        textureId = 32;
        setBoundingBoxSpacing(0.02f, 0.02f);
        scale *= random.nextFloat() * 0.6f + 0.2f;
        this->velocityX = velocityX * 0.2 + (random.nextFloat() * 2.0f - 1.0f) * 0.02f;
        this->velocityY = velocityY * 0.2 + (random.nextFloat() * 2.0f - 1.0f) * 0.02f;
        this->velocityZ = velocityZ * 0.2 + (random.nextFloat() * 2.0f - 1.0f) * 0.02f;
        maxParticleAge = static_cast<int>(8.0f / (random.nextFloat() * 0.8f + 0.2f));
    }

    void tick() override {
        prevX = x;
        prevY = y;
        prevZ = z;
        velocityY += 0.002;
        move(velocityX, velocityY, velocityZ);
        velocityX *= 0.85;
        velocityY *= 0.85;
        velocityZ *= 0.85;
        if (world != nullptr && &world->getMaterial(MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z)) !=
                                    &block::material::Material::WATER) {
            markDead();
        }
        if (maxParticleAge-- <= 0) {
            markDead();
        }
    }
};
}  // namespace net::minecraft::client::particle
