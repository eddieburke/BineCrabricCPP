#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"

namespace net::minecraft::client::render::entity {

class GhastEntityRenderer : public LivingEntityRenderer {
public:
    GhastEntityRenderer();

protected:
    void applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) override;
};

} // namespace net::minecraft::client::render::entity
