#include "net/minecraft/world/mutation/BlockMutationModule.hpp"
#include <optional>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft {
World& BlockMutationModule::world() noexcept {
  return context_.world;
}
const World& BlockMutationModule::world() const noexcept {
  return context_.world;
}
bool BlockMutationModule::setBlock(int x, int y, int z, int blockId, std::uint8_t meta) {
  if(y < 0 || y >= Chunk::height) {
    return false;
  }
  Chunk& chunk = world().ensureChunk(x, z);
  if(!chunk.setBlock(mod_16(x), y, mod_16(z), blockId, meta)) {
    return false;
  }
  blockUpdate(x, y, z, blockId);
  return true;
}
void BlockMutationModule::setBlockMeta(int x, int y, int z, int meta) {
  if(!setBlockMetaWithoutNotifyingNeighbors(x, y, z, meta)) {
    return;
  }
  const int blockId = world().getBlockId(x, y, z);
  if(blockId <= 0 || blockId >= block::Block::BLOCK_COUNT) {
    return;
  }
  if(block::Block::BLOCKS_IGNORE_META_UPDATE[static_cast<std::size_t>(blockId)]) {
    blockUpdate(x, y, z, blockId);
  } else {
    notifyNeighbors(x, y, z, blockId);
  }
}
bool BlockMutationModule::setBlockMetaWithoutNotifyingNeighbors(int x, int y, int z, int meta) {
  if(y < 0 || y >= Chunk::height) {
    return false;
  }
  world().ensureChunk(x, z).setBlockMeta(mod_16(x), y, mod_16(z), meta);
  return true;
}
bool BlockMutationModule::setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId, int meta) {
  if(y < 0 || y >= Chunk::height) {
    return false;
  }
  return world().ensureChunk(x, z).setBlock(mod_16(x), y, mod_16(z), blockId, static_cast<std::uint8_t>(meta));
}
bool BlockMutationModule::setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId) {
  return setBlockWithoutNotifyingNeighbors(x, y, z, blockId, 0);
}
void BlockMutationModule::blockUpdate(int x, int y, int z, int blockId) {
  world().blockUpdateEvent(x, y, z);
  notifyNeighbors(x, y, z, blockId);
}
void BlockMutationModule::neighborUpdate(int x, int y, int z, int sourceBlockId) {
  if(world().pauseTicking || world().isRemote()) {
    return;
  }
  const int blockId = world().getBlockId(x, y, z);
  if(blockId <= 0 || blockId >= block::Block::BLOCK_COUNT) {
    return;
  }
  block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
  if(block != nullptr) {
    block->neighborUpdate(&world(), x, y, z, sourceBlockId);
  }
}
void BlockMutationModule::notifyNeighbors(int x, int y, int z, int blockId) {
  neighborUpdate(x - 1, y, z, blockId);
  neighborUpdate(x + 1, y, z, blockId);
  neighborUpdate(x, y - 1, z, blockId);
  neighborUpdate(x, y + 1, z, blockId);
  neighborUpdate(x, y, z - 1, blockId);
  neighborUpdate(x, y, z + 1, blockId);
}
bool BlockMutationModule::canPlace(int blockId, int x, int y, int z, bool fallingBlock, int side) {
  if(blockId <= 0 || blockId >= block::Block::BLOCK_COUNT) {
    return false;
  }
  block::Block* blockAtPos = block::Block::BLOCKS[static_cast<std::size_t>(world().getBlockId(x, y, z))];
  block::Block* blockToPlace = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
  if(blockToPlace == nullptr) {
    return false;
  }
  std::optional<Box> box = fallingBlock ? std::nullopt : blockToPlace->getCollisionShape(&world(), x, y, z);
  if(box.has_value() && !world().canSpawnEntity(*box)) {
    return false;
  }
  if(blockAtPos == block::Block::FLOWING_WATER || blockAtPos == block::Block::WATER ||
     blockAtPos == block::Block::FLOWING_LAVA || blockAtPos == block::Block::LAVA ||
     blockAtPos == block::Block::FIRE || blockAtPos == block::Block::SNOW) {
    blockAtPos = nullptr;
  }
  return blockAtPos == nullptr && blockToPlace->canPlaceAt(&world(), x, y, z, side);
}
} // namespace net::minecraft
