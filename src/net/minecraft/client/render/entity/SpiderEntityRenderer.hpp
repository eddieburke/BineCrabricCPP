#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"

namespace net::minecraft::client::render::entity {

class SpiderEntityRenderer : public LivingEntityRenderer {
public:
    SpiderEntityRenderer();

protected:
    float getDeathYaw(const net::minecraft::LivingEntity& entity) const override;
    bool bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) override;
};

} // namespace net::minecraft::client::render::entity
