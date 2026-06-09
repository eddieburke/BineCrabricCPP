#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

namespace net::minecraft::client::render::entity {

class GiantEntityRenderer : public LivingEntityRenderer {
public:
    GiantEntityRenderer(model::EntityModel* model, float shadowSize, float scale);

protected:
    void applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) override;

private:
    float scale_ = 1.0f;
};

} // namespace net::minecraft::client::render::entity
