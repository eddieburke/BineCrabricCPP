#pragma once
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::particle {
class FootstepParticle : public Particle {
public:
  FootstepParticle(texture::TextureManager* textureManager, World* world, double x, double y, double z)
      : Particle(world, x, y, z, 0.0, 0.0, 0.0), textureManager_(textureManager) {
    velocityX = velocityY = velocityZ = 0.0;
    footstepMaxAge = 200;
  }
  void render(render::Tessellator& tessellator, float partialTicks, float, float, float, float, float) override {
    if(textureManager_ == nullptr || world == nullptr) {
      return;
    }
    float progress = (static_cast<float>(footstepAge) + partialTicks) / static_cast<float>(footstepMaxAge);
    progress *= progress;
    float alpha = 2.0f - progress * 2.0f;
    if(alpha > 1.0f) {
      alpha = 1.0f;
    }
    alpha *= 0.2f;
    const float size = 0.125f;
    const float px = static_cast<float>(x - xOffset);
    const float py = static_cast<float>(y - yOffset);
    const float pz = static_cast<float>(z - zOffset);
    const float brightness =
        world->getLightBrightness(MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z));
    textureManager_->bindTexture(textureManager_->getTextureId("/misc/footprint.png"));
    gl::GL11::glDisable(gl::GL11::GL_FOG);
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
    tessellator.startQuads();
    tessellator.color(brightness, brightness, brightness, alpha);
    tessellator.vertex(px - size, py, pz + size, 0.0, 1.0);
    tessellator.vertex(px + size, py, pz + size, 1.0, 1.0);
    tessellator.vertex(px + size, py, pz - size, 1.0, 0.0);
    tessellator.vertex(px - size, py, pz - size, 0.0, 0.0);
    tessellator.draw();
    gl::GL11::glDisable(gl::GL11::GL_BLEND);
    gl::GL11::glEnable(gl::GL11::GL_LIGHTING);
  }
  void tick() override {
    if(++footstepAge == footstepMaxAge) {
      markDead();
    }
  }
  [[nodiscard]] int getGroup() const override {
    return 3;
  }
  int footstepAge = 0;
  int footstepMaxAge = 0;

private:
  texture::TextureManager* textureManager_ = nullptr;
};
} // namespace net::minecraft::client::particle
