#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/ChestBlockEntity.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/block/entity/JukeboxBlockEntity.hpp"
#include "net/minecraft/block/entity/MobSpawnerBlockEntity.hpp"
#include "net/minecraft/block/entity/NoteBlockBlockEntity.hpp"
#include "net/minecraft/block/entity/PistonBlockEntity.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/registry/ContentRegistries.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/events/GameEventListener.hpp"
namespace net::minecraft::block::entity {
void BlockEntity::readNbt(const NbtCompound& nbt) {
  x = nbt.getInt("x");
  y = nbt.getInt("y");
  z = nbt.getInt("z");
}
void BlockEntity::writeNbt(NbtCompound& nbt) const {
  nbt.putString("id", id());
  nbt.putInt("x", x);
  nbt.putInt("y", y);
  nbt.putInt("z", z);
}
int BlockEntity::getPushedBlockData() const {
  if(world == nullptr) {
    return 0;
  }
  return static_cast<int>(world->getBlockMeta(x, y, z));
}
Block* BlockEntity::getBlock() const {
  if(world == nullptr) {
    return nullptr;
  }
  const int blockId = world->getBlockId(x, y, z);
  if(blockId < 0 || blockId >= Block::BLOCK_COUNT) {
    return nullptr;
  }
  return Block::BLOCKS[static_cast<std::size_t>(blockId)];
}
void BlockEntity::markDirty() {
  if(world != nullptr) {
    world->updateBlockEntity(x, y, z, this);
  }
}
std::unique_ptr<Packet> BlockEntity::createUpdatePacket() const {
  return nullptr;
}
std::unique_ptr<BlockEntity> BlockEntity::createFromNbt(const NbtCompound& nbt) {
  const std::string typeId = nbt.getString("id");
  std::unique_ptr<BlockEntity> blockEntity = registry::BlockEntityRegistry::instance().create(typeId);
  if(blockEntity == nullptr) {
    return nullptr;
  }
  blockEntity->readNbt(nbt);
  return blockEntity;
}
} // namespace net::minecraft::block::entity
