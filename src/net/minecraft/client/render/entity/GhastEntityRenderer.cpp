#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/render/entity/model/GhastEntityModel.hpp"
#include "net/minecraft/entity/mob/GhastEntity.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
namespace net::minecraft::client::render::entity {
GhastEntityRenderer::GhastEntityRenderer() : LivingEntityRenderer(new model::GhastEntityModel(), 0.5f) {
}
void GhastEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) {
 const auto* ghast = dynamic_cast<const ::net::minecraft::entity::mob::GhastEntity*>(&entity);
 if(ghast == nullptr) {
  return;
 }
 float charge = (static_cast<float>(ghast->lastChargeTime) +
                 static_cast<float>(ghast->chargeTime - ghast->lastChargeTime) * tickDelta) /
                20.0f;
 if(charge < 0.0f) {
  charge = 0.0f;
 }
 charge = 1.0f / (charge * charge * charge * charge * charge * 2.0f + 1.0f);
 const float scaleY = (8.0f + charge) / 2.0f;
 const float scaleXZ = (8.0f + 1.0f / charge) / 2.0f;
 RenderSystem::scale(scaleXZ, scaleY, scaleXZ);
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/mob/GhastEntity.hpp"
namespace net::minecraft::entity::mob {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> GhastEntity::ClientRenderer::create() {
 return std::make_unique<::net::minecraft::client::render::entity::GhastEntityRenderer>();
}
} // namespace net::minecraft::entity::mob
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::mob::GhastEntity> autoRendererReg;
} // namespace
