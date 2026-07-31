#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/entity/passive/SquidEntity.hpp"
namespace net::minecraft::client::render::entity {
void SquidEntityRenderer::applyHandSwingRotation(const net::minecraft::LivingEntity& entity,
                                                 float headBob,
                                                 float bodyYaw,
                                                 float tickDelta,
                                                 net::minecraft::util::math::MatrixStack& matrices) {
 const auto* squid = dynamic_cast<const net::minecraft::entity::passive::SquidEntity*>(&entity);
 if(squid == nullptr) {
  LivingEntityRenderer::applyHandSwingRotation(entity, headBob, bodyYaw, tickDelta, matrices);
  return;
 }
 const float tilt = squid->lastTiltAngle + (squid->tiltAngle - squid->lastTiltAngle) * tickDelta;
 const float roll = squid->lastRollAngle + (squid->rollAngle - squid->lastRollAngle) * tickDelta;
 matrices.translate(0.0f, 0.5f, 0.0f);
 matrices.rotate(180.0f - bodyYaw, 0.0f, 1.0f, 0.0f);
 matrices.rotate(tilt, 1.0f, 0.0f, 0.0f);
 matrices.rotate(roll, 0.0f, 1.0f, 0.0f);
 matrices.translate(0.0f, -1.2f, 0.0f);
}
void SquidEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta,
                                     net::minecraft::util::math::MatrixStack& matrices) {
 (void)entity;
 (void)tickDelta;
 (void)matrices;
}
float SquidEntityRenderer::getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const {
 const auto* squid = dynamic_cast<const net::minecraft::entity::passive::SquidEntity*>(&entity);
 if(squid == nullptr) {
  return LivingEntityRenderer::getHeadBob(entity, tickDelta);
 }
 return squid->lastTentacleAngle + (squid->tentacleAngle - squid->lastTentacleAngle) * tickDelta;
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/client/render/entity/model/SquidEntityModel.hpp"
#include "net/minecraft/entity/passive/SquidEntity.hpp"
namespace net::minecraft::entity::passive {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> SquidEntity::ClientRenderer::create() {
 using namespace ::net::minecraft::client::render::entity;
 using namespace ::net::minecraft::client::render::entity::model;
 return std::make_unique<SquidEntityRenderer>(new SquidEntityModel(), 0.7f);
}
} // namespace net::minecraft::entity::passive
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::passive::SquidEntity> autoRendererReg;
} // namespace
