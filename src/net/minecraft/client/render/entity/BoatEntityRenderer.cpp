#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/render/entity/model/BoatEntityModel.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity {
BoatEntityRenderer::BoatEntityRenderer() {
  shadowRadius = 0.5f;
  model_ = new model::BoatEntityModel();
}
BoatEntityRenderer::~BoatEntityRenderer() {
  delete model_;
  model_ = nullptr;
}
void BoatEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) {
  const auto* boat = dynamic_cast<const net::minecraft::entity::vehicle::BoatEntity*>(&entity);
  if(boat == nullptr || model_ == nullptr) {
    return;
  }
  gl::pushMatrix();
  gl::translatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
  gl::rotatef(180.0f - yaw, 0.0f, 1.0f, 0.0f);
  float wobbleTicks = static_cast<float>(boat->damageWobbleTicks) - tickDelta;
  float wobbleStrength = boat->damageWobbleStrength - tickDelta;
  if(wobbleStrength < 0.0f) {
    wobbleStrength = 0.0f;
  }
  if(wobbleTicks > 0.0f) {
    gl::rotatef(MathHelper::sin(wobbleTicks) * wobbleTicks * wobbleStrength / 10.0f *
                    static_cast<float>(boat->damageWobbleSide),
                1.0f,
                0.0f,
                0.0f);
  }
  bindTexture("/terrain.png");
  constexpr float scalePass = 0.75f;
  gl::scalef(scalePass, scalePass, scalePass);
  gl::scalef(1.0f / scalePass, 1.0f / scalePass, 1.0f / scalePass);
  bindTexture("/item/boat.png");
  gl::scalef(-1.0f, -1.0f, 1.0f);
  model_->render(0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 0.0625f);
  gl::popMatrix();
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
namespace net::minecraft::entity::vehicle {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> BoatEntity::ClientRenderer::create() {
  return std::make_unique<::net::minecraft::client::render::entity::BoatEntityRenderer>();
}
} // namespace net::minecraft::entity::vehicle
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::vehicle::BoatEntity> autoRendererReg;
} // namespace
