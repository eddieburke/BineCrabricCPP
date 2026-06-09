#pragma once

#include "net/minecraft/client/render/entity/EntityRenderer.hpp"

namespace net::minecraft::client::render::entity {

class LightningEntityRenderer : public EntityRenderer {
public:
    using EntityRenderer::EntityRenderer;
    void render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) override;
};

} // namespace net::minecraft::client::render::entity
