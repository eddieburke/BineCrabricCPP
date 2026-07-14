#pragma once
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
namespace net::minecraft::client::particle {
class PickupParticle : public Particle {
public:
  PickupParticle(World* world, Entity* entity, Entity* collector, float offsetY)
      : Particle(world,
                 entity != nullptr ? entity->x : 0.0,
                 entity != nullptr ? entity->y : 0.0,
                 entity != nullptr ? entity->z : 0.0,
                 entity != nullptr ? entity->velocityX : 0.0,
                 entity != nullptr ? entity->velocityY : 0.0,
                 entity != nullptr ? entity->velocityZ : 0.0),
        entity_(entity),
        collector_(collector),
        offsetY_(offsetY) {
    lifetime_ = 3;
  }
  void render(render::Tessellator& /*tessellator*/,
              float partialTicks,
              float /*horizontalSize*/,
              float /*verticalSize*/,
              float /*depthSize*/,
              float /*widthOffset*/,
              float /*heightOffset*/) override {
    if(entity_ == nullptr || collector_ == nullptr || world == nullptr) {
      return;
    }
    float progress = (static_cast<float>(pickupAge_) + partialTicks) / static_cast<float>(lifetime_);
    progress *= progress;
    const double sourceX = entity_->x;
    const double sourceY = entity_->y;
    const double sourceZ = entity_->z;
    const double targetX = collector_->lastTickX + (collector_->x - collector_->lastTickX) * partialTicks;
    const double targetY =
        collector_->lastTickY + (collector_->y - collector_->lastTickY) * partialTicks + offsetY_;
    const double targetZ = collector_->lastTickZ + (collector_->z - collector_->lastTickZ) * partialTicks;
    double renderX = sourceX + (targetX - sourceX) * progress;
    double renderY = sourceY + (targetY - sourceY) * progress;
    double renderZ = sourceZ + (targetZ - sourceZ) * progress;
    const int blockX = MathHelper::floor(renderX);
    const int blockY = MathHelper::floor(renderY + static_cast<double>(standingEyeHeight / 2.0f));
    const int blockZ = MathHelper::floor(renderZ);
    const float brightness = world->getLightBrightness(blockX, blockY, blockZ);
    gl::color4f(brightness, brightness, brightness, 1.0f);
    render::entity::EntityRenderDispatcher::instance().render(
        *entity_, renderX - xOffset, renderY - yOffset, renderZ - zOffset, entity_->yaw, partialTicks);
  }
  void tick() override {
    ++pickupAge_;
    if(pickupAge_ == lifetime_) {
      markDead();
    }
  }
  [[nodiscard]] int getGroup() const override {
    return 3;
  }

private:
  Entity* entity_ = nullptr;
  Entity* collector_ = nullptr;
  int pickupAge_ = 0;
  int lifetime_ = 0;
  float offsetY_ = 0.0f;
};
} // namespace net::minecraft::client::particle
