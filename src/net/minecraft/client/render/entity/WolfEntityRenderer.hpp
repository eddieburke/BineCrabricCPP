#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

namespace net::minecraft::client::render::entity {

class WolfEntityRenderer : public LivingEntityRenderer {
public:
    WolfEntityRenderer(model::EntityModel* model, float shadowRadius);

protected:
    float getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const override;
    void applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) override;
};

} // namespace net::minecraft::client::render::entity
