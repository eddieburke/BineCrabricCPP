#pragma once

#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"

namespace net::minecraft::client::render::entity {

class TntEntityRenderer : public EntityRenderer {
public:
    TntEntityRenderer();

    void render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) override;

private:
    block::BlockRenderManager blockRenderManager_;
};

} // namespace net::minecraft::client::render::entity
