#include "net/minecraft/client/render/entity/SheepEntityRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/EntityRendererCasts.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"

namespace net::minecraft::client::render::entity {

SheepEntityRenderer::SheepEntityRenderer(model::EntityModel* model, model::EntityModel* furModel, float shadowRadius)
    : LivingEntityRenderer(model, shadowRadius)
{
    setDecorationModel(furModel);
}

bool SheepEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta)
{
    const auto* sheep = dynamic_cast<const casts::SheepEntity*>(&entity);
    if (sheep == nullptr || layer != 0 || sheep->isSheared()) {
        return false;
    }
    EntityRenderer::bindTexture("/mob/sheep_fur.png");
    const int colorIndex = sheep->getColor() & 0xF;
    const float brightness = sheep->getBrightnessAtEyes(tickDelta);
    const float* tint = casts::SheepEntity::COLORS[colorIndex];
    gl::GL11::glColor3f(tint[0] * brightness, tint[1] * brightness, tint[2] * brightness);
    return true;
}

} // namespace net::minecraft::client::render::entity
