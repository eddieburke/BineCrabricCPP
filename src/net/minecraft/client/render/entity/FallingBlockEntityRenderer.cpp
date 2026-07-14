#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::render::entity {
FallingBlockEntityRenderer::FallingBlockEntityRenderer() {
  shadowRadius = 0.5f;
}
void FallingBlockEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) {
  (void)yaw;
  (void)tickDelta;
  const auto* falling = dynamic_cast<const net::minecraft::FallingBlockEntity*>(&entity);
  if(falling == nullptr) {
    return;
  }
  const gl::MatrixGuard matrix;
  gl::translatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
  bindTexture("/terrain.png");
  net::minecraft::block::Block* block =
      net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(falling->blockId)];
  if(block != nullptr) {
    const platform::LightingOffGuard lighting;
    blockRenderManager_.renderFallingBlockEntity(*block,
                                                 falling->world,
                                                 MathHelper::floor(falling->x),
                                                 MathHelper::floor(falling->y),
                                                 MathHelper::floor(falling->z));
  }
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
namespace net::minecraft::entity {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> FallingBlockEntity::ClientRenderer::create() {
  return std::make_unique<::net::minecraft::client::render::entity::FallingBlockEntityRenderer>();
}
} // namespace net::minecraft::entity
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::FallingBlockEntity> autoRendererReg;
} // namespace
