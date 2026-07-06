#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity {
void PaintingEntityRenderer::render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw,
                                    float tickDelta) {
  (void)tickDelta;
  const auto* painting = dynamic_cast<const ::net::minecraft::entity::decoration::painting::PaintingEntity*>(&entity);
  if(painting == nullptr) {
    return;
  }
  random_.seed(187ULL);
  const gl::MatrixGuard matrix;
  gl::GL11::glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
  gl::GL11::glRotatef(yaw, 0.0f, 1.0f, 0.0f);
  bindTexture("/art/kz.png");
  constexpr float scale = 0.0625f;
  gl::GL11::glScalef(scale, scale, scale);
  renderPainting(*painting, painting->variant.width, painting->variant.height, painting->variant.textureOffsetX,
                 painting->variant.textureOffsetY);
}
void PaintingEntityRenderer::renderPainting(
    const ::net::minecraft::entity::decoration::painting::PaintingEntity& painting, int width, int height, int u, int v) {
  const float left = static_cast<float>(-width) / 2.0f;
  const float top = static_cast<float>(-height) / 2.0f;
  constexpr float back = -0.5f;
  constexpr float front = 0.5f;
  for(int tileX = 0; tileX < width / 16; ++tileX) {
    for(int tileY = 0; tileY < height / 16; ++tileY) {
      const float right = left + static_cast<float>((tileX + 1) * 16);
      const float tileLeft = left + static_cast<float>(tileX * 16);
      const float bottom = top + static_cast<float>((tileY + 1) * 16);
      const float tileTop = top + static_cast<float>(tileY * 16);
      applyBrightness(painting, (right + tileLeft) / 2.0f, (bottom + tileTop) / 2.0f);
      const float uMax = static_cast<float>(u + width - tileX * 16) / 256.0f;
      const float uMin = static_cast<float>(u + width - (tileX + 1) * 16) / 256.0f;
      const float vMax = static_cast<float>(v + height - tileY * 16) / 256.0f;
      const float vMin = static_cast<float>(v + height - (tileY + 1) * 16) / 256.0f;
      Tessellator& tessellator = Tessellator::INSTANCE;
      tessellator.startQuads();
      tessellator.normal(0.0f, 0.0f, -1.0f);
      tessellator.vertex(right, tileTop, back, uMin, vMax);
      tessellator.vertex(tileLeft, tileTop, back, uMax, vMax);
      tessellator.vertex(tileLeft, bottom, back, uMax, vMin);
      tessellator.vertex(right, bottom, back, uMin, vMin);
      tessellator.normal(0.0f, 0.0f, 1.0f);
      tessellator.vertex(right, bottom, front, 0.75f, 0.0f);
      tessellator.vertex(tileLeft, bottom, front, 0.8125f, 0.0f);
      tessellator.vertex(tileLeft, tileTop, front, 0.8125f, 0.0625f);
      tessellator.vertex(right, tileTop, front, 0.75f, 0.0625f);
      tessellator.normal(0.0f, -1.0f, 0.0f);
      tessellator.vertex(right, bottom, back, 0.75f, 0.001953125f);
      tessellator.vertex(tileLeft, bottom, back, 0.8125f, 0.001953125f);
      tessellator.vertex(tileLeft, bottom, front, 0.8125f, 0.001953125f);
      tessellator.vertex(right, bottom, front, 0.75f, 0.001953125f);
      tessellator.normal(0.0f, 1.0f, 0.0f);
      tessellator.vertex(right, tileTop, front, 0.75f, 0.001953125f);
      tessellator.vertex(tileLeft, tileTop, front, 0.8125f, 0.001953125f);
      tessellator.vertex(tileLeft, tileTop, back, 0.8125f, 0.001953125f);
      tessellator.vertex(right, tileTop, back, 0.75f, 0.001953125f);
      tessellator.normal(-1.0f, 0.0f, 0.0f);
      tessellator.vertex(right, bottom, front, 0.7519531f, 0.0f);
      tessellator.vertex(right, tileTop, front, 0.7519531f, 0.0625f);
      tessellator.vertex(right, tileTop, back, 0.7519531f, 0.0625f);
      tessellator.vertex(right, bottom, back, 0.7519531f, 0.0f);
      tessellator.normal(1.0f, 0.0f, 0.0f);
      tessellator.vertex(tileLeft, bottom, back, 0.7519531f, 0.0f);
      tessellator.vertex(tileLeft, tileTop, back, 0.7519531f, 0.0625f);
      tessellator.vertex(tileLeft, tileTop, front, 0.7519531f, 0.0625f);
      tessellator.vertex(tileLeft, bottom, front, 0.7519531f, 0.0f);
      tessellator.draw();
    }
  }
}
void PaintingEntityRenderer::applyBrightness(
    const ::net::minecraft::entity::decoration::painting::PaintingEntity& painting, float u, float v) {
  int bx = MathHelper::floor(painting.x);
  int by = MathHelper::floor(painting.y + static_cast<double>(v / 16.0f));
  int bz = MathHelper::floor(painting.z);
  if(painting.facing == 0) {
    bx = MathHelper::floor(painting.x + static_cast<double>(u / 16.0f));
  }
  if(painting.facing == 1) {
    bz = MathHelper::floor(painting.z - static_cast<double>(u / 16.0f));
  }
  if(painting.facing == 2) {
    bx = MathHelper::floor(painting.x - static_cast<double>(u / 16.0f));
  }
  if(painting.facing == 3) {
    bz = MathHelper::floor(painting.z + static_cast<double>(u / 16.0f));
  }
  float brightness = 0.0f;
  if(dispatcher != nullptr && dispatcher->world() != nullptr) {
    brightness = dispatcher->world()->getLightBrightness(bx, by, bz);
  }
  gl::GL11::glColor3f(brightness, brightness, brightness);
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
namespace net::minecraft::entity::decoration::painting {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> PaintingEntity::ClientRenderer::create() {
  return std::make_unique<::net::minecraft::client::render::entity::PaintingEntityRenderer>();
}
} // namespace net::minecraft::entity::decoration::painting
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::decoration::painting::PaintingEntity>
    autoRendererReg;
} // namespace
