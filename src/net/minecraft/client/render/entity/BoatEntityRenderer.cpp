#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
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
    const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta,
    net::minecraft::util::math::MatrixStack& matrices, const net::minecraft::util::math::Matrix4f& projection) {
 const auto* boat = dynamic_cast<const net::minecraft::entity::vehicle::BoatEntity*>(&entity);
 if(boat == nullptr || model_ == nullptr) {
  return;
 }
 beginDraw(matrices, projection);
 matrices.push();
 matrices.translate(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
 matrices.rotate(180.0f - yaw, 0.0f, 1.0f, 0.0f);
 float wobbleTicks = static_cast<float>(boat->damageWobbleTicks) - tickDelta;
 float wobbleStrength = boat->damageWobbleStrength - tickDelta;
 if(wobbleStrength < 0.0f) {
  wobbleStrength = 0.0f;
 }
 if(wobbleTicks > 0.0f) {
  matrices.rotate(MathHelper::sin(wobbleTicks) * wobbleTicks * wobbleStrength / 10.0f *
                      static_cast<float>(boat->damageWobbleSide),
                  1.0f,
                  0.0f,
                  0.0f);
 }
 bindTexture("/terrain.png");
 constexpr float scalePass = 0.75f;
 matrices.scale(scalePass, scalePass, scalePass);
 matrices.scale(1.0f / scalePass, 1.0f / scalePass, 1.0f / scalePass);
 bindTexture("/item/boat.png");
 matrices.scale(-1.0f, -1.0f, 1.0f);
 render::core::setDrawPose(matrices.top());
 model_->render(0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 0.0625f);
 matrices.pop();
 endDraw();
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
