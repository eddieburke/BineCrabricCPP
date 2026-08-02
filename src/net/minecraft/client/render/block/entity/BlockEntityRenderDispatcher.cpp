#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/block/Block.hpp"
namespace net::minecraft::client::render::block::entity {
BlockEntityRenderDispatcher& BlockEntityRenderDispatcher::instance() {
 static BlockEntityRenderDispatcher dispatcher;
 return dispatcher;
}
BlockEntityRenderDispatcher::BlockEntityRenderDispatcher() {
 registerRenderer<net::minecraft::block::entity::SignBlockEntity>(std::make_unique<SignBlockEntityRenderer>());
 registerRenderer<net::minecraft::block::entity::MobSpawnerBlockEntity>(
     std::make_unique<MobSpawnerBlockEntityRenderer>());
 registerRenderer<net::minecraft::block::entity::PistonBlockEntity>(std::make_unique<PistonBlockEntityRenderer>());
}
void BlockEntityRenderDispatcher::prepare(net::minecraft::World* worldIn,
                                          ::net::minecraft::client::texture::TextureManager* textureManagerIn,
                                          font::TextRenderer* textRendererIn,
                                          const net::minecraft::Entity* cameraIn,
                                          float tickDelta) {
 if(world != worldIn) {
  setWorld(worldIn);
 }
 textureManager = textureManagerIn;
 textRenderer = textRendererIn;
 camera = cameraIn;
 if(camera != nullptr) {
  cameraYaw = camera->prevYaw + (camera->yaw - camera->prevYaw) * tickDelta;
  cameraPitch = camera->prevPitch + (camera->pitch - camera->prevPitch) * tickDelta;
  cameraX = camera->lastTickX + (camera->x - camera->lastTickX) * static_cast<double>(tickDelta);
  cameraY = camera->lastTickY + (camera->y - camera->lastTickY) * static_cast<double>(tickDelta);
  cameraZ = camera->lastTickZ + (camera->z - camera->lastTickZ) * static_cast<double>(tickDelta);
 } else {
  cameraYaw = 0.0f;
  cameraPitch = 0.0f;
  cameraX = cameraY = cameraZ = 0.0;
 }
}
void BlockEntityRenderDispatcher::render(const net::minecraft::block::entity::BlockEntity& blockEntity,
                                         float tickDelta) {
 if(blockEntity.distanceFrom(cameraX, cameraY, cameraZ) < 4096.0) {
  // MCP TileEntityRenderer.renderTileEntity: glColor3f(getLightBrightness(x,y,z)).
  const float brightness =
      world != nullptr ? world->getLightBrightness(blockEntity.x, blockEntity.y, blockEntity.z) : 1.0f;
  Tessellator::INSTANCE.light(15, 15);
  render::core::setConstColor(brightness, brightness, brightness, 1.0f);
   int blockEntityShaderId = -1;
   if(world != nullptr) {
    const int blockId = world->getBlockId(blockEntity.x, blockEntity.y, blockEntity.z);
    if(blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr) {
     Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
     std::string name = block->getTranslationKey();
     if(name.rfind("tile.", 0) == 0) name.erase(0, 5);
     // Java defaults unresolved block ids to -1 (BlockMaterialMapping
     // defaultReturnValue(-1)); a mapped id is a superset of the vanilla -1.
     blockEntityShaderId = resolveShaderObjectId("block", PackCatalog::lower(std::move(name)), blockId);
    }
   }
  const render::core::BlockEntityIdScope blockEntityIdScope(blockEntityShaderId);
  {
   const RenderPassScope passScope(RenderType::block());
   const render::core::ScopedDrawCameraState drawGuard;
   render(blockEntity,
          static_cast<double>(blockEntity.x) - offsetX,
          static_cast<double>(blockEntity.y) - offsetY,
          static_cast<double>(blockEntity.z) - offsetZ,
          tickDelta);
  }
 }
}
} // namespace net::minecraft::client::render::block::entity
