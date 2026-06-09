#pragma once

#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::client::render::entity {

class ItemEntityRenderer : public EntityRenderer {
public:
    ItemEntityRenderer();

    void render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) override;

private:
    block::BlockRenderManager blockRenderManager_;
    JavaRandom random_ {187L};
    bool useCustomDisplayColor_ = true;
};

} // namespace net::minecraft::client::render::entity
