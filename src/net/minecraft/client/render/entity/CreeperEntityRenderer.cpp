#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/entity/model/CreeperEntityModel.hpp"
#include "net/minecraft/entity/mob/CreeperEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include <algorithm>
namespace net::minecraft::client::render::entity {
CreeperEntityRenderer::CreeperEntityRenderer() : LivingEntityRenderer(new model::CreeperEntityModel(), 0.5f) {
  chargedModel_ = new model::CreeperEntityModel(2.0f);
}
CreeperEntityRenderer::~CreeperEntityRenderer() {
  delete chargedModel_;
  chargedModel_ = nullptr;
}
void CreeperEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) {
  const auto* creeper = dynamic_cast<const net::minecraft::entity::mob::CreeperEntity*>(&entity);
  float swell = creeper != nullptr ? creeper->getScale(tickDelta) : 0.0f;
  float pulse = 1.0f + net::minecraft::util::math::MathHelper::sin(swell * 100.0f) * swell * 0.01f;
  if(swell < 0.0f)
    swell = 0.0f;
  if(swell > 1.0f)
    swell = 1.0f;
  swell *= swell;
  swell *= swell;
  const float sx = (1.0f + swell * 0.4f) * pulse;
  const float sy = (1.0f + swell * 0.1f) / pulse;
  gl::scalef(sx, sy, sx);
}
int CreeperEntityRenderer::getOverlayColor(const net::minecraft::LivingEntity& entity, float /*brightness*/,
                                           float tickDelta) const {
  const auto* creeper = dynamic_cast<const net::minecraft::entity::mob::CreeperEntity*>(&entity);
  const float swell = creeper != nullptr ? creeper->getScale(tickDelta) : 0.0f;
  if(static_cast<int>(swell * 10.0f) % 2 == 0)
    return 0;
  int alpha = static_cast<int>(swell * 0.2f * 255.0f);
  alpha = std::clamp(alpha, 0, 255);
  return (alpha << 24) | (255 << 16) | (255 << 8) | 255;
}
bool CreeperEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) {
  const auto* creeper = dynamic_cast<const net::minecraft::entity::mob::CreeperEntity*>(&entity);
  if(creeper == nullptr || !creeper->isCharged())
    return false;
  if(layer == 1) {
    const float time = static_cast<float>(entity.age) + tickDelta;
    EntityRenderer::bindTexture("/armor/power.png");
    gl::matrixMode(5890);
    gl::loadIdentity();
    const float offset = time * 0.01f;
    gl::translatef(offset, offset, 0.0f);
    setDecorationModel(chargedModel_);
    gl::matrixMode(5888);
    gl::setCap(gl::cap::Blend, true);
    gl::color4f(0.5f, 0.5f, 0.5f, 1.0f);
    gl::blendFunc(gl::blend::One, gl::blend::One);
    return true;
  }
  if(layer == 2) {
    gl::matrixMode(5890);
    gl::loadIdentity();
    gl::matrixMode(5888);
    gl::setCap(gl::cap::Blend, false);
  }
  return false;
}
bool CreeperEntityRenderer::bindDecorationTexture(const net::minecraft::LivingEntity& /*entity*/, int /*layer*/,
                                                  float /*tickDelta*/) {
  return false;
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/mob/CreeperEntity.hpp"
namespace net::minecraft::entity::mob {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> CreeperEntity::ClientRenderer::create() {
  return std::make_unique<::net::minecraft::client::render::entity::CreeperEntityRenderer>();
}
} // namespace net::minecraft::entity::mob
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::mob::CreeperEntity> autoRendererReg;
} // namespace
