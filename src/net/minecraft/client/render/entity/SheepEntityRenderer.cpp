#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/EntityRendererCasts.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
namespace net::minecraft::client::render::entity {
SheepEntityRenderer::SheepEntityRenderer(model::EntityModel* model, model::EntityModel* furModel, float shadowRadius)
    : LivingEntityRenderer(model, shadowRadius) {
  setDecorationModel(furModel);
}
bool SheepEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) {
  const auto* sheep = dynamic_cast<const casts::SheepEntity*>(&entity);
  if(sheep == nullptr || layer != 0 || sheep->isSheared()) {
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
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/client/render/entity/model/SheepEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/SheepWoolEntityModel.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
namespace net::minecraft::entity::passive {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> SheepEntity::ClientRenderer::create() {
  using namespace ::net::minecraft::client::render::entity;
  using namespace ::net::minecraft::client::render::entity::model;
  return std::make_unique<SheepEntityRenderer>(new SheepEntityModel(), new SheepWoolEntityModel(), 0.7f);
}
} // namespace net::minecraft::entity::passive
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::passive::SheepEntity> autoRendererReg;
} // namespace
