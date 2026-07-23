#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/render/entity/model/SpiderEntityModel.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/mob/SpiderEntity.hpp"
namespace net::minecraft::client::render::entity {
SpiderEntityRenderer::SpiderEntityRenderer() : LivingEntityRenderer(new model::SpiderEntityModel(), 1.0f) {
 setDecorationModel(new model::SpiderEntityModel());
}
float SpiderEntityRenderer::getDeathYaw(const net::minecraft::LivingEntity& entity) const {
 (void)entity;
 return 180.0f;
}
bool SpiderEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) {
 if(layer != 0) {
  return false;
 }
 EntityRenderer::bindTexture("/mob/spider_eyes.png");
 const float brightness = entity.getBrightnessAtEyes(1.0f);
 const float alpha = (1.0f - brightness) * 0.5f;
 RenderSystem::blendAlpha();
 RenderSystem::alphaTest(0.0f);
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, alpha);
 (void)tickDelta;
 return true;
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/mob/SpiderEntity.hpp"
namespace net::minecraft::entity::mob {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> SpiderEntity::ClientRenderer::create() {
 return std::make_unique<::net::minecraft::client::render::entity::SpiderEntityRenderer>();
}
} // namespace net::minecraft::entity::mob
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::mob::SpiderEntity> autoRendererReg;
} // namespace
