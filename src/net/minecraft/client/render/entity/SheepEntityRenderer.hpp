#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

namespace net::minecraft::client::render::entity {

class SheepEntityRenderer : public LivingEntityRenderer {
public:
    SheepEntityRenderer(model::EntityModel* model, model::EntityModel* furModel, float shadowRadius);

protected:
    bool bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) override;
};

} // namespace net::minecraft::client::render::entity
