#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/passive/PigEntity.hpp"

namespace net::minecraft::client::render::entity {
PigEntityRenderer::PigEntityRenderer(model::EntityModel* model, model::EntityModel* saddleModel, float shadowRadius)
    : LivingEntityRenderer(model, shadowRadius) {
    setDecorationModel(saddleModel);
}

bool PigEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) {
    (void) tickDelta;
    const auto* pig = dynamic_cast<const ::net::minecraft::entity::passive::PigEntity*>(&entity);
    if (pig == nullptr || layer != 0 || !pig->isSaddled()) {
        return false;
    }
    EntityRenderer::bindTexture("/mob/saddle.png");
    return true;
}
}  // namespace net::minecraft::client::render::entity

#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/client/render/entity/model/PigEntityModel.hpp"
#include "net/minecraft/entity/passive/PigEntity.hpp"

namespace net::minecraft::entity::passive {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> PigEntity::ClientRenderer::create() {
    using namespace ::net::minecraft::client::render::entity;
    using namespace ::net::minecraft::client::render::entity::model;
    return std::make_unique<PigEntityRenderer>(new PigEntityModel(), new PigEntityModel(0.5f), 0.7f);
}
}  // namespace net::minecraft::entity::passive

namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::passive::PigEntity> autoRendererReg;
}  // namespace
