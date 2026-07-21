#pragma once
#include "net/minecraft/client/particle/Particle.hpp"
namespace net::minecraft::client::particle {
class NoteParticle : public Particle {
 public:
 NoteParticle(World* world,
              double x,
              double y,
              double z,
              double noteColorModifier,
              double velocityY,
              double velocityZ,
              float scaleIn = 2.0f)
     : Particle(world, x, y, z, 0.0, 0.0, 0.0) {
  (void)velocityY;
  (void)velocityZ;
  this->velocityX *= 0.01;
  this->velocityY = this->velocityY * 0.01 + 0.2;
  this->velocityZ *= 0.01;
  red = MathHelper::sin((static_cast<float>(noteColorModifier) + 0.0f) * kPiF * 2.0f) * 0.65f + 0.35f;
  green = MathHelper::sin((static_cast<float>(noteColorModifier) + 0.33333334f) * kPiF * 2.0f) * 0.65f + 0.35f;
  blue = MathHelper::sin((static_cast<float>(noteColorModifier) + 0.6666667f) * kPiF * 2.0f) * 0.65f + 0.35f;
  scale *= 0.75f * scaleIn;
  startScale = scale;
  maxParticleAge = 6;
  textureId = 64;
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
  scale = startScale * std::clamp(f, 0.0f, 1.0f);
  Particle::render(tessellator, partialTicks, horizontalSize, verticalSize, depthSize, widthOffset, heightOffset);
 }
 float startScale = 1.0f;
};
} // namespace net::minecraft::client::particle
