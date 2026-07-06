#include "net/minecraft/client/render/entity/ProjectileEntityRenderer.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/entity/Entity.hpp"
namespace net::minecraft::client::render::entity {
ProjectileEntityRenderer::ProjectileEntityRenderer(int itemTextureIdIn) : itemTextureId(itemTextureIdIn) {}
void ProjectileEntityRenderer::render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw,
                                      float tickDelta) {
  (void)entity;
  (void)yaw;
  (void)tickDelta;
  const gl::MatrixGuard matrix;
  gl::GL11::glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
  gl::GL11::glScalef(0.5f, 0.5f, 0.5f);
  bindTexture("/gui/items.png");
  Tessellator& tessellator = Tessellator::INSTANCE;
  const float uMin = static_cast<float>((itemTextureId % 16 * 16) + 0) / 256.0f;
  const float uMax = static_cast<float>((itemTextureId % 16 * 16) + 16) / 256.0f;
  const float vMin = static_cast<float>((itemTextureId / 16 * 16) + 0) / 256.0f;
  const float vMax = static_cast<float>((itemTextureId / 16 * 16) + 16) / 256.0f;
  constexpr float size = 1.0f;
  constexpr float half = 0.5f;
  constexpr float quarter = 0.25f;
  if(dispatcher != nullptr) {
    gl::GL11::glRotatef(180.0f - dispatcher->yaw_, 0.0f, 1.0f, 0.0f);
    gl::GL11::glRotatef(-dispatcher->pitch_, 1.0f, 0.0f, 0.0f);
  }
  tessellator.startQuads();
  tessellator.normal(0.0f, 1.0f, 0.0f);
  tessellator.vertex(0.0f - half, 0.0f - quarter, 0.0, uMin, vMax);
  tessellator.vertex(size - half, 0.0f - quarter, 0.0, uMax, vMax);
  tessellator.vertex(size - half, size - quarter, 0.0, uMax, vMin);
  tessellator.vertex(0.0f - half, size - quarter, 0.0, uMin, vMin);
  tessellator.draw();
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/projectile/thrown/EggEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
namespace net::minecraft::entity::projectile::thrown {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> SnowballEntity::ClientRenderer::create() {
  return std::make_unique<::net::minecraft::client::render::entity::ProjectileEntityRenderer>(14);
}
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> EggEntity::ClientRenderer::create() {
  return std::make_unique<::net::minecraft::client::render::entity::ProjectileEntityRenderer>(12);
}
} // namespace net::minecraft::entity::projectile::thrown
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<::net::minecraft::entity::projectile::thrown::SnowballEntity>
    snowballRendererReg;
static ::net::minecraft::registry::RegisterEntityRenderer<::net::minecraft::entity::projectile::thrown::EggEntity>
    eggRendererReg;
} // namespace
