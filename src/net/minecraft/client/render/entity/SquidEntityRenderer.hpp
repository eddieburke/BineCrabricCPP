#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"

namespace net::minecraft::client::render::entity {

class SquidEntityRenderer : public LivingEntityRenderer {
public:
    using LivingEntityRenderer::LivingEntityRenderer;

protected:
    void applyHandSwingRotation(const net::minecraft::LivingEntity& entity, float headBob, float bodyYaw,
        float tickDelta) override;
    void applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) override;
    [[nodiscard]] float getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const override;
};

} // namespace net::minecraft::client::render::entity
