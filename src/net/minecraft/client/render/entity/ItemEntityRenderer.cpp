#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/client/render/TextureResolve.hpp"
#include "net/minecraft/mod/runtime/LuaRenderBindings.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
namespace net::minecraft::client::render::entity {
namespace {
struct MatrixScope {
 MatrixScope() { RenderSystem::pushMatrix(); }
 ~MatrixScope() { RenderSystem::popMatrix(); }
 MatrixScope(const MatrixScope&) = delete;
 MatrixScope& operator=(const MatrixScope&) = delete;
};
} // namespace
ItemEntityRenderer::ItemEntityRenderer() {
 shadowRadius = 0.15f;
 shadowDarkness = 0.75f;
}
void ItemEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float /*yaw*/, float tickDelta) {
 if(net::minecraft::mod::runtime::itemModelRenderOverrideActive()) {
  return;
 }
 const auto* itemEntity = dynamic_cast<const net::minecraft::ItemEntity*>(&entity);
 if(itemEntity == nullptr || itemEntity->stack.empty()) {
  return;
 }
 random_.setSeed(187L);
 const ItemStack& stack = itemEntity->stack;
 const RenderPassScope passScope(RenderType::entityCutout());
 const MatrixScope matrix;
 const float bob = MathHelper::sin((static_cast<float>(itemEntity->itemAge) + tickDelta) / 10.0f +
                                   itemEntity->initialRotationAngle) *
                       0.1f +
                   0.1f;
 const float spin =
     ((static_cast<float>(itemEntity->itemAge) + tickDelta) / 20.0f + itemEntity->initialRotationAngle) * 57.295776f;
 int duplicateCount = 1;
 if(stack.count > 1) {
  duplicateCount = 2;
 }
 if(stack.count > 5) {
  duplicateCount = 3;
 }
 if(stack.count > 20) {
  duplicateCount = 4;
 }
 RenderSystem::translate(static_cast<float>(x), static_cast<float>(y) + bob, static_cast<float>(z));
 if(item::ItemModelRenderer::rendersAsBlockModel(stack)) {
  RenderSystem::rotate(spin, 0.0f, 1.0f, 0.0f);
  net::minecraft::block::Block* block = item::ItemModelRenderer::blockOf(stack);
   if(block != nullptr && dispatcher != nullptr && dispatcher->textureManager() != nullptr) {
   const client::render::ResolvedTexture resolved = client::render::resolveBlockTexture(
       block->textureId, *dispatcher->textureManager(), client::render::AtlasDomain::Terrain);
   if(resolved.glId >= 0) {
    RenderSystem::bindTexture(0x0DE1, resolved.glId);
   }
  } else {
   bindTexture("/terrain.png");
  }
  float scale = 0.25f;
  if(block != nullptr && !block->isFullCube() && stack.itemId != 44 && block->getRenderType() != 16) {
   scale = 0.5f;
  }
  RenderSystem::scale(scale, scale, scale);
  for(int i = 0; i < duplicateCount; ++i) {
   RenderSystem::pushMatrix();
   if(i > 0) {
    const float offsetX = (random_.nextFloat() * 2.0f - 1.0f) * 0.2f / scale;
    const float offsetY = (random_.nextFloat() * 2.0f - 1.0f) * 0.2f / scale;
    const float offsetZ = (random_.nextFloat() * 2.0f - 1.0f) * 0.2f / scale;
    RenderSystem::translate(offsetX, offsetY, offsetZ);
   }
   if(block != nullptr) {
    blockRenderManager_.render(*block, stack.getDamage(), entity.getBrightnessAtEyes(tickDelta));
   }
   RenderSystem::popMatrix();
  }
 } else {
  RenderSystem::scale(0.5f, 0.5f, 0.5f);
  const int textureId = stack.getTextureId();
  const bool isBlockSprite =
      stack.itemId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(stack.itemId)] != nullptr;
   if(dispatcher != nullptr && dispatcher->textureManager() != nullptr) {
   const client::render::ResolvedTexture resolved = client::render::resolveBlockTexture(
       textureId, *dispatcher->textureManager(), isBlockSprite ? client::render::AtlasDomain::Terrain
                                                                : client::render::AtlasDomain::Items);
   if(resolved.glId >= 0) {
    RenderSystem::activeTexture(0x84C0);
    RenderSystem::enableTexture();
    RenderSystem::bindTexture(0x0DE1, resolved.glId);
   }
  } else if(isBlockSprite) {
   bindTexture("/terrain.png");
  } else {
   bindTexture("/gui/items.png");
  }
  Tessellator& tessellator = Tessellator::INSTANCE;
  const client::render::ResolvedTexture uv = client::render::resolveBlockTextureUv(textureId);
  const float uMin = static_cast<float>(uv.uMin);
  const float uMax = static_cast<float>(uv.uMax);
  const float vMin = static_cast<float>(uv.vMin);
  const float vMax = static_cast<float>(uv.vMax);
  constexpr float width = 1.0f;
  constexpr float halfWidth = 0.5f;
  constexpr float quarterHeight = 0.25f;
  const float brightness = entity.getBrightnessAtEyes(tickDelta);
  if(useCustomDisplayColor_ && stack.getItem() != nullptr) {
   const item::ItemTint tint = item::ItemModelRenderer::tintColor(stack);
   RenderSystem::color4f(tint.red * brightness, tint.green * brightness, tint.blue * brightness, 1.0f);
  } else {
   RenderSystem::color4f(brightness, brightness, brightness, 1.0f);
  }
  for(int i = 0; i < duplicateCount; ++i) {
   RenderSystem::pushMatrix();
   if(i > 0) {
    const float offsetX = (random_.nextFloat() * 2.0f - 1.0f) * 0.3f;
    const float offsetY = (random_.nextFloat() * 2.0f - 1.0f) * 0.3f;
    const float offsetZ = (random_.nextFloat() * 2.0f - 1.0f) * 0.3f;
    RenderSystem::translate(offsetX, offsetY, offsetZ);
   }
   if(dispatcher != nullptr) {
    RenderSystem::rotate(180.0f - dispatcher->yaw_, 0.0f, 1.0f, 0.0f);
   }
   tessellator.startQuads();
   tessellator.normal(0.0f, 1.0f, 0.0f);
   tessellator.vertex(0.0f - halfWidth, 0.0f - quarterHeight, 0.0, uMin, vMax);
   tessellator.vertex(width - halfWidth, 0.0f - quarterHeight, 0.0, uMax, vMax);
   tessellator.vertex(width - halfWidth, 1.0f - quarterHeight, 0.0, uMax, vMin);
   tessellator.vertex(0.0f - halfWidth, 1.0f - quarterHeight, 0.0, uMin, vMin);
   tessellator.draw();
   RenderSystem::popMatrix();
  }
 }
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
namespace net::minecraft::entity {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> ItemEntity::ClientRenderer::create() {
 return std::make_unique<::net::minecraft::client::render::entity::ItemEntityRenderer>();
}
} // namespace net::minecraft::entity
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::ItemEntity> autoRendererReg;
} // namespace
