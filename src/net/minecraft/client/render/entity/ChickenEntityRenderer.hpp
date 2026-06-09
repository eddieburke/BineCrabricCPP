#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"

namespace net::minecraft::client::render::entity {

class ChickenEntityRenderer : public LivingEntityRenderer {
public:
    using LivingEntityRenderer::LivingEntityRenderer;

protected:
    [[nodiscard]] float getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const override;
};

} // namespace net::minecraft::client::render::entity
