#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
namespace net::minecraft::client::render::entity {
GiantEntityRenderer::GiantEntityRenderer(model::EntityModel* model, float shadowSize, float scale)
    : LivingEntityRenderer(model, shadowSize * scale), scale_(scale) {
}
void GiantEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) {
  (void)entity;
  (void)tickDelta;
  gl::scalef(scale_, scale_, scale_);
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/client/render/entity/model/ZombieEntityModel.hpp"
#include "net/minecraft/entity/mob/GiantEntity.hpp"
namespace net::minecraft::entity::mob {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> GiantEntity::ClientRenderer::create() {
  using namespace ::net::minecraft::client::render::entity;
  using namespace ::net::minecraft::client::render::entity::model;
  return std::make_unique<GiantEntityRenderer>(new ZombieEntityModel(), 0.5f, 6.0f);
}
} // namespace net::minecraft::entity::mob
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::mob::GiantEntity> autoRendererReg;
} // namespace
