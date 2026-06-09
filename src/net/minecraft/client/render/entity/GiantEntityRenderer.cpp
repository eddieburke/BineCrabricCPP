#include "net/minecraft/client/render/entity/GiantEntityRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"

namespace net::minecraft::client::render::entity {

GiantEntityRenderer::GiantEntityRenderer(model::EntityModel* model, float shadowSize, float scale)
    : LivingEntityRenderer(model, shadowSize * scale)
    , scale_(scale)
{
}

void GiantEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta)
{
    (void)entity;
    (void)tickDelta;
    gl::GL11::glScalef(scale_, scale_, scale_);
}

} // namespace net::minecraft::client::render::entity
