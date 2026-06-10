#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

namespace net::minecraft::client::render::entity {

class CreeperEntityRenderer : public LivingEntityRenderer {
public:
    CreeperEntityRenderer();
    ~CreeperEntityRenderer() override;

protected:
    void applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) override;
    int getOverlayColor(const net::minecraft::LivingEntity& entity, float brightness, float tickDelta) const override;
    bool bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) override;
    bool bindDecorationTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) override;

private:
    model::EntityModel* chargedModel_ = nullptr;
};

} // namespace net::minecraft::client::render::entity
