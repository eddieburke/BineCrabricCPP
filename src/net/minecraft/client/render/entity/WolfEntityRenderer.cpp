#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/entity/passive/WolfEntity.hpp"
namespace net::minecraft::client::render::entity {
WolfEntityRenderer::WolfEntityRenderer(model::EntityModel* model, float shadowRadius)
    : LivingEntityRenderer(model, shadowRadius) {
}
float WolfEntityRenderer::getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const {
 if(const auto* wolf = dynamic_cast<const net::minecraft::entity::passive::WolfEntity*>(&entity); wolf != nullptr) {
  return wolf->getTailAngle();
 }
 return LivingEntityRenderer::getHeadBob(entity, tickDelta);
}
void WolfEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) {
 (void)entity;
 (void)tickDelta;
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/client/render/entity/model/WolfEntityModel.hpp"
#include "net/minecraft/entity/passive/WolfEntity.hpp"
namespace net::minecraft::entity::passive {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> WolfEntity::ClientRenderer::create() {
 using namespace ::net::minecraft::client::render::entity;
 using namespace ::net::minecraft::client::render::entity::model;
 return std::make_unique<WolfEntityRenderer>(new WolfEntityModel(), 0.5f);
}
} // namespace net::minecraft::entity::passive
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::passive::WolfEntity> autoRendererReg;
} // namespace
