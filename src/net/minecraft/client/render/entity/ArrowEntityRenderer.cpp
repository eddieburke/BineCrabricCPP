#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity {
void ArrowEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float /*yaw*/, float tickDelta) {
  const auto* arrow = dynamic_cast<const net::minecraft::entity::projectile::ArrowEntity*>(&entity);
  if(arrow == nullptr) {
    return;
  }
  if(arrow->prevYaw == 0.0f && arrow->prevPitch == 0.0f) {
    return;
  }
  bindTexture("/item/arrows.png");
  const gl::MatrixGuard matrix;
  gl::translatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
  gl::rotatef(arrow->prevYaw + (arrow->yaw - arrow->prevYaw) * tickDelta - 90.0f, 0.0f, 1.0f, 0.0f);
  gl::rotatef(arrow->prevPitch + (arrow->pitch - arrow->prevPitch) * tickDelta, 0.0f, 0.0f, 1.0f);
  Tessellator& tessellator = Tessellator::INSTANCE;
  constexpr int arrowVariant = 0;
  constexpr float shaftUMin = 0.0f;
  constexpr float shaftUMax = 0.5f;
  const float shaftVMin = static_cast<float>(0 + arrowVariant * 10) / 32.0f;
  const float shaftVMax = static_cast<float>(5 + arrowVariant * 10) / 32.0f;
  constexpr float headUMin = 0.0f;
  constexpr float headUMax = 0.15625f;
  const float headVMin = static_cast<float>(5 + arrowVariant * 10) / 32.0f;
  const float headVMax = static_cast<float>(10 + arrowVariant * 10) / 32.0f;
  constexpr float scale = 0.05625f;
  const float shake = arrow->shake - tickDelta;
  if(shake > 0.0f) {
    const float shakeAngle = -MathHelper::sin(shake * 3.0f) * shake;
    gl::rotatef(shakeAngle, 0.0f, 0.0f, 1.0f);
  }
  gl::rotatef(45.0f, 1.0f, 0.0f, 0.0f);
  gl::scalef(scale, scale, scale);
  gl::translatef(-4.0f, 0.0f, 0.0f);
  gl::normal3f(scale, 0.0f, 0.0f);
  tessellator.startQuads();
  tessellator.vertex(-7.0, -2.0, -2.0, headUMin, headVMin);
  tessellator.vertex(-7.0, -2.0, 2.0, headUMax, headVMin);
  tessellator.vertex(-7.0, 2.0, 2.0, headUMax, headVMax);
  tessellator.vertex(-7.0, 2.0, -2.0, headUMin, headVMax);
  tessellator.draw();
  gl::normal3f(-scale, 0.0f, 0.0f);
  tessellator.startQuads();
  tessellator.vertex(-7.0, 2.0, -2.0, headUMin, headVMin);
  tessellator.vertex(-7.0, 2.0, 2.0, headUMax, headVMin);
  tessellator.vertex(-7.0, -2.0, 2.0, headUMax, headVMax);
  tessellator.vertex(-7.0, -2.0, -2.0, headUMin, headVMax);
  tessellator.draw();
  for(int i = 0; i < 4; ++i) {
    gl::rotatef(90.0f, 1.0f, 0.0f, 0.0f);
    gl::normal3f(0.0f, 0.0f, scale);
    tessellator.startQuads();
    tessellator.vertex(-8.0, -2.0, 0.0, shaftUMin, shaftVMin);
    tessellator.vertex(8.0, -2.0, 0.0, shaftUMax, shaftVMin);
    tessellator.vertex(8.0, 2.0, 0.0, shaftUMax, shaftVMax);
    tessellator.vertex(-8.0, 2.0, 0.0, shaftUMin, shaftVMax);
    tessellator.draw();
  }
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
namespace net::minecraft::entity::projectile {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> ArrowEntity::ClientRenderer::create() {
  return std::make_unique<::net::minecraft::client::render::entity::ArrowEntityRenderer>();
}
} // namespace net::minecraft::entity::projectile
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::projectile::ArrowEntity>
    autoRendererReg;
} // namespace
