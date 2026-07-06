#include "net/minecraft/mod/ModClient.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include <cassert>
namespace net::minecraft::mod {
namespace {
BlockWorldDraw* worldDraws() {
  static BlockWorldDraw table[block::Block::BLOCK_COUNT]{};
  return table;
}
BlockInvDraw* inventoryDraws() {
  static BlockInvDraw table[block::Block::BLOCK_COUNT]{};
  return table;
}
} // namespace
void registerDraw(int blockId, BlockWorldDraw world, BlockInvDraw inventory) {
  if(blockId < 0 || blockId >= block::Block::BLOCK_COUNT) {
    return;
  }
  if(world != nullptr) {
    assert(worldDraws()[blockId] == nullptr);
    worldDraws()[blockId] = world;
  }
  if(inventory != nullptr) {
    assert(inventoryDraws()[blockId] == nullptr);
    inventoryDraws()[blockId] = inventory;
  }
}
bool drawBlockWorld(client::render::block::BlockRenderManager& manager, block::Block& block, int x, int y, int z) {
  if(block.id < 0 || block.id >= block::Block::BLOCK_COUNT) {
    return false;
  }
  BlockWorldDraw draw = worldDraws()[block.id];
  return draw != nullptr && draw(manager, block, x, y, z);
}
bool drawBlockInventory(client::render::block::BlockRenderManager& manager, block::Block& block, int metadata,
                        float brightness) {
  if(block.id < 0 || block.id >= block::Block::BLOCK_COUNT) {
    return false;
  }
  BlockInvDraw draw = inventoryDraws()[block.id];
  if(draw == nullptr) {
    return false;
  }
  draw(manager, block, metadata, brightness);
  return true;
}
} // namespace net::minecraft::mod
