#include "net/minecraft/client/render/block/entity/PistonBlockEntityRenderer.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PistonBlock.hpp"
#include "net/minecraft/block/PistonHeadBlock.hpp"
#include "net/minecraft/block/entity/PistonBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/gl/Lighting.hpp"
namespace net::minecraft::client::render::block::entity {
void PistonBlockEntityRenderer::setWorld(net::minecraft::World* world) {
  blockRenderManager.setBlockView(world);
}
void PistonBlockEntityRenderer::render(
    const net::minecraft::block::entity::BlockEntity& blockEntity, double x, double y, double z, float tickDelta) {
  const auto* piston = dynamic_cast<const net::minecraft::block::entity::PistonBlockEntity*>(&blockEntity);
  if(piston == nullptr) {
    return;
  }
  net::minecraft::block::Block* block =
      net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(piston->getPushedBlockId())];
  if(block == nullptr || piston->getProgress(tickDelta) >= 1.0f) {
    return;
  }
  Tessellator& tessellator = render::INSTANCE;
  bindTexture("/terrain.png");
  gl::Lighting::turnOff();
  const gl::preset::PistonTranslucent pistonCaps;
  tessellator.startQuads();
  tessellator.translate(static_cast<double>(static_cast<float>(x) - static_cast<float>(piston->x) +
                                            piston->getRenderOffsetX(tickDelta)),
                        static_cast<double>(static_cast<float>(y) - static_cast<float>(piston->y) +
                                            piston->getRenderOffsetY(tickDelta)),
                        static_cast<double>(static_cast<float>(z) - static_cast<float>(piston->z) +
                                            piston->getRenderOffsetZ(tickDelta)));
  tessellator.color(1, 1, 1);
  if(block == net::minecraft::block::Block::PISTON_HEAD && piston->getProgress(tickDelta) < 0.5f) {
    blockRenderManager.renderPistonHeadWithoutCulling(block->id, piston->x, piston->y, piston->z, false);
  } else if(piston->isSource() && !piston->isExtending()) {
    auto* pistonBlock = dynamic_cast<net::minecraft::block::PistonBlock*>(block);
    if(pistonBlock != nullptr && net::minecraft::block::Block::PISTON_HEAD != nullptr) {
      const int blockMeta = blockRenderManager.ctx.blockView->getBlockMeta(piston->x, piston->y, piston->z);
      const int facing = net::minecraft::block::PistonHeadBlock::getFacing(blockMeta);
      blockRenderManager.ctx.faceTextureOverride = pistonBlock->getTopTexture();
      blockRenderManager.ctx.faceTextureSide = facing;
      blockRenderManager.renderPistonHeadWithoutCulling(net::minecraft::block::Block::PISTON_HEAD->id,
                                                        piston->x,
                                                        piston->y,
                                                        piston->z,
                                                        piston->getProgress(tickDelta) < 0.5f);
      blockRenderManager.ctx.faceTextureOverride = -1;
      blockRenderManager.ctx.faceTextureSide = -1;
      tessellator.translate(static_cast<double>(static_cast<float>(x) - static_cast<float>(piston->x)),
                            static_cast<double>(static_cast<float>(y) - static_cast<float>(piston->y)),
                            static_cast<double>(static_cast<float>(z) - static_cast<float>(piston->z)));
      blockRenderManager.renderExtendedPiston(block->id, piston->x, piston->y, piston->z);
    }
  } else {
    blockRenderManager.renderWithoutCulling(block->id, piston->x, piston->y, piston->z);
  }
  tessellator.translate(0.0, 0.0, 0.0);
  tessellator.draw();
  gl::Lighting::turnOn();
}
} // namespace net::minecraft::client::render::block::entity
