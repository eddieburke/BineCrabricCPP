#pragma once
#include "net/minecraft/client/particle/Particle.hpp"

namespace net::minecraft::client::particle {
class FlameParticle : public Particle {
   public:
    FlameParticle(World* world, double x, double y, double z, double velocityX, double velocityY, double velocityZ)
        : Particle(world, x, y, z, velocityX, velocityY, velocityZ) {
        this->velocityX = this->velocityX * 0.01 + velocityX;
        this->velocityY = this->velocityY * 0.01 + velocityY;
        this->velocityZ = this->velocityZ * 0.01 + velocityZ;
        this->x += (random.nextFloat() - random.nextFloat()) * 0.05f;
        this->y += (random.nextFloat() - random.nextFloat()) * 0.05f;
        this->z += (random.nextFloat() - random.nextFloat()) * 0.05f;
        initialScale = scale;
        red = green = blue = 1.0f;
        maxParticleAge = static_cast<int>(8.0f / (random.nextFloat() * 0.8f + 0.2f)) + 4;
        noClip = true;
        textureId = 48;
    }

    float getBrightnessAtEyes(float tickDelta) const override {
        float f = maxParticleAge > 0
                      ? (static_cast<float>(particleAge) + tickDelta) / static_cast<float>(maxParticleAge)
                      : 1.0f;
        f = std::clamp(f, 0.0f, 1.0f);
        const float base = Particle::getBrightnessAtEyes(tickDelta);
        return base * f + (1.0f - f);
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
                            : 0.0f;
        scale = initialScale * (1.0f - f * f * 0.5f);
        Particle::render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
    }

    void tick() override {
        prevX = x;
        prevY = y;
        prevZ = z;
        if (particleAge++ >= maxParticleAge) {
            markDead();
        }
        move(velocityX, velocityY, velocityZ);
        velocityX *= 0.96;
        velocityY *= 0.96;
        velocityZ *= 0.96;
        if (onGround) {
            velocityX *= 0.7;
            velocityZ *= 0.7;
        }
    }

    float initialScale = 1.0f;
};
}  // namespace net::minecraft::client::particle
