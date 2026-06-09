#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

namespace net::minecraft::client::render::entity {

class SlimeEntityRenderer : public LivingEntityRenderer {
public:
    SlimeEntityRenderer(model::EntityModel* model, model::EntityModel* innerModel, float shadowRadius);

protected:
    [[nodiscard]] bool bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) override;
    void applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) override;
};

} // namespace net::minecraft::client::render::entity
