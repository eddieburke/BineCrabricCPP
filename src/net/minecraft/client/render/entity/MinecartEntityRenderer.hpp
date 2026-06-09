#pragma once

#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

namespace net::minecraft::client::render::entity {

class MinecartEntityRenderer : public EntityRenderer {
public:
    MinecartEntityRenderer();
    ~MinecartEntityRenderer() override;

    void render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) override;

private:
    model::EntityModel* model_ = nullptr;
    block::BlockRenderManager blockRenderManager_;
};

} // namespace net::minecraft::client::render::entity
