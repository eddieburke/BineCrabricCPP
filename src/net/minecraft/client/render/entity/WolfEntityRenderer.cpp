#include "net/minecraft/client/render/entity/WolfEntityRenderer.hpp"

#include "net/minecraft/entity/passive/WolfEntity.hpp"

namespace net::minecraft::client::render::entity {

WolfEntityRenderer::WolfEntityRenderer(model::EntityModel* model, float shadowRadius)
    : LivingEntityRenderer(model, shadowRadius)
{
}

float WolfEntityRenderer::getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const
{
    if (const auto* wolf = dynamic_cast<const net::minecraft::entity::passive::WolfEntity*>(&entity); wolf != nullptr) {
        return wolf->getTailAngle();
    }
    return LivingEntityRenderer::getHeadBob(entity, tickDelta);
}

void WolfEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta)
{
    (void)entity;
    (void)tickDelta;
}

} // namespace net::minecraft::client::render::entity
