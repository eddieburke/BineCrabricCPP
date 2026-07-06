#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/client/render/entity/model/CowEntityModel.hpp"
#include "net/minecraft/entity/passive/CowEntity.hpp"
namespace net::minecraft::entity::passive {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> CowEntity::ClientRenderer::create() {
  using net::minecraft::client::render::entity::CowEntityRenderer;
  using net::minecraft::client::render::entity::model::CowEntityModel;
  return std::make_unique<CowEntityRenderer>(new CowEntityModel(), 0.7f);
}
} // namespace net::minecraft::entity::passive
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<::net::minecraft::entity::passive::CowEntity> autoRendererReg;
} // namespace
namespace net::minecraft::client::render::entity {
CowEntityRenderer::CowEntityRenderer(model::EntityModel* model, float shadowRadius)
    : LivingEntityRenderer(model, shadowRadius) {
}
} // namespace net::minecraft::client::render::entity
