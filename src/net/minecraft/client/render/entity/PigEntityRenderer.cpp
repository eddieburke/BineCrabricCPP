#include "net/minecraft/client/render/entity/PigEntityRenderer.hpp"

#include "net/minecraft/entity/passive/PigEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"

namespace net::minecraft::client::render::entity {

PigEntityRenderer::PigEntityRenderer(model::EntityModel* model, model::EntityModel* saddleModel, float shadowRadius)
    : LivingEntityRenderer(model, shadowRadius)
{
    setDecorationModel(saddleModel);
}

bool PigEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta)
{
    (void)tickDelta;
    const auto* pig = dynamic_cast<const ::net::minecraft::entity::passive::PigEntity*>(&entity);
    if (pig == nullptr || layer != 0 || !pig->isSaddled()) {
        return false;
    }
    EntityRenderer::bindTexture("/mob/saddle.png");
    return true;
}

} // namespace net::minecraft::client::render::entity
