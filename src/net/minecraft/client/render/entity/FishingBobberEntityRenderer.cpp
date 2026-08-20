#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Vec3dClient.hpp"
namespace net::minecraft::client::render::entity {
void FishingBobberEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta,
    net::minecraft::util::math::MatrixStack& matrices, const net::minecraft::util::math::Matrix4f& projection) {
 (void)yaw;
 const auto* bobber = dynamic_cast<const ::net::minecraft::entity::projectile::FishingBobberEntity*>(&entity);
 if(bobber == nullptr) {
  return;
 }
 beginDraw(matrices, projection);
 // Nothing binds a shader program for the entity stage; a draw submitted with
 // none bound is dropped silently (RenderCore submit). Every entity renderer
 // opens its own pass -- LivingEntityRenderer does, and its scope restores the
 // previous (unbound) program on exit, so inheriting one is never an option.
 const render::RenderPassScope spritePass(render::RenderType::entityCutout());
 matrices.push();
 matrices.translate(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
 matrices.scale(0.5f, 0.5f, 0.5f);
 constexpr int atlasCol = 1;
 constexpr int atlasRow = 2;
 bindTexture("/particles.png");
 Tessellator& tessellator = Tessellator::INSTANCE;
 const float uMin = static_cast<float>(atlasCol * 8 + 0) / 128.0f;
 const float uMax = static_cast<float>(atlasCol * 8 + 8) / 128.0f;
 const float vMin = static_cast<float>(atlasRow * 8 + 0) / 128.0f;
 const float vMax = static_cast<float>(atlasRow * 8 + 8) / 128.0f;
 constexpr float quadWidth = 1.0f;
 constexpr float halfWidth = 0.5f;
 constexpr float halfHeight = 0.5f;
 if(dispatcher != nullptr) {
  matrices.rotate(180.0f - dispatcher->yaw_, 0.0f, 1.0f, 0.0f);
  matrices.rotate(-dispatcher->pitch_, 1.0f, 0.0f, 0.0f);
 }
 render::core::setDrawPose(matrices.top());
 tessellator.startQuads();
 tessellator.normal(0.0f, 1.0f, 0.0f);
 tessellator.vertex(0.0f - halfWidth, 0.0f - halfHeight, 0.0, uMin, vMax);
 tessellator.vertex(quadWidth - halfWidth, 0.0f - halfHeight, 0.0, uMax, vMax);
 tessellator.vertex(quadWidth - halfWidth, quadWidth - halfHeight, 0.0, uMax, vMin);
 tessellator.vertex(0.0f - halfWidth, quadWidth - halfHeight, 0.0, uMin, vMin);
 tessellator.draw();
 matrices.pop();
 if(bobber->owner == nullptr || dispatcher == nullptr) {
  endDraw();
  return;
 }
 const net::minecraft::LivingEntity& owner = *bobber->owner;
 float ownerYaw = (owner.prevYaw + (owner.yaw - owner.prevYaw) * tickDelta) * kPiF / 180.0f;
 double sinYaw = MathHelper::sin(ownerYaw);
 double cosYaw = MathHelper::cos(ownerYaw);
 const float swing = owner.getHandSwingProgress(tickDelta);
 const float swingBob = MathHelper::sin(MathHelper::sqrt(swing) * kPiF);
 net::minecraft::util::math::ClientVec3d vec{-0.5, 0.03, 0.8};
 vec.rotateX(-(owner.prevPitch + (owner.pitch - owner.prevPitch) * tickDelta) * kPiF / 180.0f);
 vec.rotateY(-(owner.prevYaw + (owner.yaw - owner.prevYaw) * tickDelta) * kPiF / 180.0f);
 vec.rotateY(swingBob * 0.5f);
 vec.rotateX(-swingBob * 0.7f);
 double rodTipX = owner.prevX + (owner.x - owner.prevX) * static_cast<double>(tickDelta) + vec.x;
 double rodTipY = owner.prevY + (owner.y - owner.prevY) * static_cast<double>(tickDelta) + vec.y;
 double rodTipZ = owner.prevZ + (owner.z - owner.prevZ) * static_cast<double>(tickDelta) + vec.z;
 if(dispatcher->options().thirdPerson) {
  ownerYaw = (owner.lastBodyYaw + (owner.bodyYaw - owner.lastBodyYaw) * tickDelta) * kPiF / 180.0f;
  sinYaw = MathHelper::sin(ownerYaw);
  cosYaw = MathHelper::cos(ownerYaw);
  rodTipX =
      owner.prevX + (owner.x - owner.prevX) * static_cast<double>(tickDelta) - cosYaw * 0.35 - sinYaw * 0.85;
  rodTipY = owner.prevY + (owner.y - owner.prevY) * static_cast<double>(tickDelta) - 0.45;
  rodTipZ =
      owner.prevZ + (owner.z - owner.prevZ) * static_cast<double>(tickDelta) - sinYaw * 0.35 + cosYaw * 0.85;
 }
 const double bobberX = bobber->prevX + (bobber->x - bobber->prevX) * static_cast<double>(tickDelta);
 const double bobberY = bobber->prevY + (bobber->y - bobber->prevY) * static_cast<double>(tickDelta) + 0.25;
 const double bobberZ = bobber->prevZ + (bobber->z - bobber->prevZ) * static_cast<double>(tickDelta);
 const double deltaX = static_cast<float>(rodTipX - bobberX);
 const double deltaY = static_cast<float>(rodTipY - bobberY);
 const double deltaZ = static_cast<float>(rodTipZ - bobberZ);
 render::RenderPassScope passScope(render::RenderType::lines());
 render::core::setEntityColor(0.0f, 0.0f, 0.0f, 0.0f);
 Tessellator::INSTANCE.light(15, 15);
 // The line draw runs on the frame base (the bobber pose popped above).
 render::core::setDrawPose(matrices.top());
 tessellator.start(3); // GL_LINE_STRIP
 tessellator.color(0);
 constexpr int segments = 16;
 for(int i = 0; i <= segments; ++i) {
  const float t = static_cast<float>(i) / static_cast<float>(segments);
  tessellator.vertex(x + deltaX * static_cast<double>(t),
                     y + deltaY * static_cast<double>(t * t + t) * 0.5 + 0.25,
                     z + deltaZ * static_cast<double>(t));
 }
 tessellator.draw();
 endDraw();
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
namespace net::minecraft::entity::projectile {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer>
FishingBobberEntity::ClientRenderer::create() {
 return std::make_unique<::net::minecraft::client::render::entity::FishingBobberEntityRenderer>();
}
} // namespace net::minecraft::entity::projectile
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::projectile::FishingBobberEntity>
    autoRendererReg;
} // namespace
