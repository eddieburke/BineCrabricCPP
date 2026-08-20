#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
namespace net::minecraft::client::render::entity {
namespace {
constexpr int kSnowballTexture = 14;
} // namespace
void FireballEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta,
    net::minecraft::util::math::MatrixStack& matrices, const net::minecraft::util::math::Matrix4f& projection) {
 (void)entity;
 (void)yaw;
 (void)tickDelta;
 beginDraw(matrices, projection);
 // Nothing binds a shader program for the entity stage; a draw submitted with
 // none bound is dropped silently (RenderCore submit). Every entity renderer
 // opens its own pass -- LivingEntityRenderer does, and its scope restores the
 // previous (unbound) program on exit, so inheriting one is never an option.
 const RenderPassScope passScope(RenderType::entityCutout());
 matrices.push();
 matrices.translate(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
 constexpr float sizeScale = 2.0f;
 matrices.scale(sizeScale, sizeScale, sizeScale);
 bindTexture("/gui/items.png");
 Tessellator& tessellator = Tessellator::INSTANCE;
 const float uMin = static_cast<float>((kSnowballTexture % 16 * 16) + 0) / 256.0f;
 const float uMax = static_cast<float>((kSnowballTexture % 16 * 16) + 16) / 256.0f;
 const float vMin = static_cast<float>((kSnowballTexture / 16 * 16) + 0) / 256.0f;
 const float vMax = static_cast<float>((kSnowballTexture / 16 * 16) + 16) / 256.0f;
 constexpr float size = 1.0f;
 constexpr float half = 0.5f;
 constexpr float quarter = 0.25f;
 if(dispatcher != nullptr) {
  matrices.rotate(180.0f - dispatcher->yaw_, 0.0f, 1.0f, 0.0f);
  matrices.rotate(-dispatcher->pitch_, 1.0f, 0.0f, 0.0f);
 }
 render::core::setDrawPose(matrices.top());
 tessellator.startQuads();
 tessellator.normal(0.0f, 1.0f, 0.0f);
 tessellator.vertex(0.0f - half, 0.0f - quarter, 0.0, uMin, vMax);
 tessellator.vertex(size - half, 0.0f - quarter, 0.0, uMax, vMax);
 tessellator.vertex(size - half, size - quarter, 0.0, uMax, vMin);
 tessellator.vertex(0.0f - half, size - quarter, 0.0, uMin, vMin);
 tessellator.draw();
 matrices.pop();
 endDraw();
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/projectile/FireballEntity.hpp"
namespace net::minecraft::entity::projectile {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> FireballEntity::ClientRenderer::create() {
 return std::make_unique<::net::minecraft::client::render::entity::FireballEntityRenderer>();
}
} // namespace net::minecraft::entity::projectile
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::projectile::FireballEntity>
    autoRendererReg;
} // namespace
