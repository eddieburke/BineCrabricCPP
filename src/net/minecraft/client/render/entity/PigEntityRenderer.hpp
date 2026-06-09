#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

namespace net::minecraft::client::render::entity {

class PigEntityRenderer : public LivingEntityRenderer {
public:
    PigEntityRenderer(model::EntityModel* model, model::EntityModel* saddleModel, float shadowRadius);

protected:
    bool bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) override;
};

} // namespace net::minecraft::client::render::entity
