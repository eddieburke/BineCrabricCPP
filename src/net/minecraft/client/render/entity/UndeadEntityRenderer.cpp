#include "net/minecraft/client/render/entity/UndeadEntityRenderer.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
namespace net::minecraft::client::render::entity {
UndeadEntityRenderer::UndeadEntityRenderer(model::BipedEntityModel* model, float shadowSize)
    : LivingEntityRenderer(model, shadowSize), entityModel_(model) {
}
void UndeadEntityRenderer::renderMore(const net::minecraft::LivingEntity& entity, float tickDelta) {
 (void)tickDelta;
 const ItemStack itemStack = entity.getHeldItem();
 if(itemStack.itemId == 0 || entityModel_ == nullptr || dispatcher == nullptr) {
  return;
 }
 RenderSystem::pushMatrix();
 entityModel_->rightArm.transform(0.0625f);
 RenderSystem::translate(-0.0625f, 0.4375f, 0.0625f);
 if(itemStack.itemId < 256) {
  net::minecraft::block::Block* block =
      net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(itemStack.itemId)];
  if(block != nullptr && block::BlockRenderManager::isSideLit(block->getRenderType())) {
   float scale = 0.5f;
   RenderSystem::translate(0.0f, 0.1875f, -0.3125f);
   RenderSystem::rotate(20.0f, 1.0f, 0.0f, 0.0f);
   RenderSystem::rotate(45.0f, 0.0f, 1.0f, 0.0f);
   scale *= 0.75f;
   RenderSystem::scale(scale, -scale, scale);
  } else if(Item* item = itemStack.getItem(); item != nullptr && item->isHandheld()) {
   float scale = 0.625f;
   RenderSystem::translate(0.0f, 0.1875f, 0.0f);
   RenderSystem::scale(scale, -scale, scale);
   RenderSystem::rotate(-100.0f, 1.0f, 0.0f, 0.0f);
   RenderSystem::rotate(45.0f, 0.0f, 1.0f, 0.0f);
  } else {
   const float scale = 0.375f;
   RenderSystem::translate(0.25f, 0.1875f, -0.1875f);
   RenderSystem::scale(scale, scale, scale);
   RenderSystem::rotate(60.0f, 0.0f, 0.0f, 1.0f);
   RenderSystem::rotate(-90.0f, 1.0f, 0.0f, 0.0f);
   RenderSystem::rotate(20.0f, 0.0f, 0.0f, 1.0f);
  }
 } else if(Item* item = itemStack.getItem(); item != nullptr && item->isHandheld()) {
  float scale = 0.625f;
  RenderSystem::translate(0.0f, 0.1875f, 0.0f);
  RenderSystem::scale(scale, -scale, scale);
  RenderSystem::rotate(-100.0f, 1.0f, 0.0f, 0.0f);
  RenderSystem::rotate(45.0f, 0.0f, 1.0f, 0.0f);
 } else {
  const float scale = 0.375f;
  RenderSystem::translate(0.25f, 0.1875f, -0.1875f);
  RenderSystem::scale(scale, scale, scale);
  RenderSystem::rotate(60.0f, 0.0f, 0.0f, 1.0f);
  RenderSystem::rotate(-90.0f, 1.0f, 0.0f, 0.0f);
  RenderSystem::rotate(20.0f, 0.0f, 0.0f, 1.0f);
 }
 dispatcher->heldItemRenderer()->renderItem(entity, itemStack);
 RenderSystem::popMatrix();
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/client/render/entity/model/SkeletonEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/ZombieEntityModel.hpp"
#include "net/minecraft/entity/mob/SkeletonEntity.hpp"
#include "net/minecraft/entity/mob/ZombieEntity.hpp"
namespace net::minecraft::entity::mob {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> SkeletonEntity::ClientRenderer::create() {
 using namespace ::net::minecraft::client::render::entity;
 using namespace ::net::minecraft::client::render::entity::model;
 return std::make_unique<UndeadEntityRenderer>(new SkeletonEntityModel(), 0.5f);
}
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> ZombieEntity::ClientRenderer::create() {
 using namespace ::net::minecraft::client::render::entity;
 using namespace ::net::minecraft::client::render::entity::model;
 return std::make_unique<UndeadEntityRenderer>(new ZombieEntityModel(), 0.5f);
}
} // namespace net::minecraft::entity::mob
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<::net::minecraft::entity::mob::SkeletonEntity>
    skeletonRendererReg;
static ::net::minecraft::registry::RegisterEntityRenderer<::net::minecraft::entity::mob::ZombieEntity>
    zombieRendererReg;
} // namespace
