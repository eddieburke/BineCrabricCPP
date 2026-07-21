#pragma once
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
namespace net::minecraft::client::render::entity {
class ProjectileEntityRenderer : public EntityRenderer {
 public:
 explicit ProjectileEntityRenderer(int itemTextureId = -1);
 void render(
     const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) override;
 int itemTextureId = -1;
};
} // namespace net::minecraft::client::render::entity
