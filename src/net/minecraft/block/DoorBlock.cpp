#include "net/minecraft/block/DoorBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
namespace {
net::minecraft::BlockSoundGroup kMetalSound("stone", 1.0f, 1.5f);
net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
} // namespace
namespace net::minecraft::block {
namespace {
void playDoorToggleSound(World* world, int x, int y, int z) {
  JavaRandom& random = world->random();
  const char* sound = random.nextDouble() < 0.5 ? "random.door_open" : "random.door_close";
  world->playSound(static_cast<double>(x) + 0.5,
                   static_cast<double>(y) + 0.5,
                   static_cast<double>(z) + 0.5,
                   sound,
                   1.0f,
                   random.nextFloat() * 0.1f + 0.9f);
}
} // namespace
DoorBlock::DoorBlock(int id, Material& mat) : Block(id, mat) {
  textureId = 97;
  if(&mat == &material::Material::METAL) {
    ++textureId;
  }
  constexpr float f = 0.5f;
  setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 1.0f, 0.5f + f);
}
int DoorBlock::facingFromMeta(int meta) {
  if((meta & 4) == 0) {
    return (meta - 1) & 3;
  }
  return meta & 3;
}
int DoorBlock::getTexture(int side, int meta) const {
  if(side == 0 || side == 1) {
    return textureId;
  }
  const int facing = facingFromMeta(meta);
  if((facing == 0 || facing == 2) ^ (side <= 3)) {
    return textureId;
  }
  int variant = facing / 2 + ((side & 1) ^ facing);
  int tex = textureId - (meta & 8) * 2;
  variant += (meta & 4) / 4;
  if((variant & 1) != 0) {
    tex = -tex;
  }
  return tex;
}
void DoorBlock::rotate(int meta) {
  setBoundingBox(boundsForMeta(meta));
}
net::minecraft::Box DoorBlock::boundsForMeta(int meta) const {
  constexpr float thickness = 0.1875f;
  const int oriented = facingFromMeta(meta);
  if(oriented == 0) {
    return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness};
  }
  if(oriented == 1) {
    return {1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
  }
  if(oriented == 2) {
    return {0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f};
  }
  return {0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f};
}
std::optional<net::minecraft::Box> DoorBlock::getCollisionShape(World* world, int x, int y, int z) const {
  if(world == nullptr) {
    return std::nullopt;
  }
  const_cast<DoorBlock*>(this)->updateBoundingBox(world, x, y, z);
  return Block::getCollisionShape(world, x, y, z);
}
void DoorBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z) {
  if(blockView == nullptr) {
    return;
  }
  rotate(blockView->getBlockMeta(x, y, z));
}
net::minecraft::Box DoorBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const {
  if(blockView == nullptr) {
    return {minX, minY, minZ, maxX, maxY, maxZ};
  }
  return boundsForMeta(blockView->getBlockMeta(x, y, z));
}
bool DoorBlock::canPlaceAt(World* world, int x, int y, int z) const {
  if(world == nullptr || y >= 127) {
    return false;
  }
  return world->shouldSuffocate(x, y - 1, z) && Block::canPlaceAt(world, x, y, z) &&
         Block::canPlaceAt(world, x, y + 1, z);
}
int DoorBlock::getDroppedItemId(int blockMeta, JavaRandom& /*random*/) const {
  if((blockMeta & 8) != 0) {
    return 0;
  }
  if(&material == &material::Material::METAL && Item::byRawId(74) != nullptr) {
    return Item::byRawId(74)->id;
  }
  return Item::byRawId(68) != nullptr ? Item::byRawId(68)->id : 324;
}
void DoorBlock::onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) {
  onUse(world, x, y, z, player);
}
bool DoorBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) {
  if(world == nullptr || &material == &material::Material::METAL) {
    return true;
  }
  int meta = world->getBlockMeta(x, y, z);
  if((meta & 8) != 0) {
    if(world->getBlockId(x, y - 1, z) == id) {
      onUse(world, x, y - 1, z, player);
    }
    return true;
  }
  if(world->getBlockId(x, y + 1, z) == id) {
    world->setBlockMeta(x, y + 1, z, (meta ^ 4) + 8);
  }
  world->setBlockMeta(x, y, z, meta ^ 4);
  world->setBlocksDirty(x, y - 1, z, x, y, z);
  playDoorToggleSound(world, x, y, z);
  return true;
}
void DoorBlock::setOpen(World* world, int x, int y, int z, bool open) {
  if(world == nullptr) {
    return;
  }
  int meta = world->getBlockMeta(x, y, z);
  if((meta & 8) != 0) {
    if(world->getBlockId(x, y - 1, z) == id) {
      setOpen(world, x, y - 1, z, open);
    }
    return;
  }
  const bool currentlyOpen = (meta & 4) != 0;
  if(currentlyOpen == open) {
    return;
  }
  if(world->getBlockId(x, y + 1, z) == id) {
    world->setBlockMeta(x, y + 1, z, (meta ^ 4) + 8);
  }
  world->setBlockMeta(x, y, z, meta ^ 4);
  world->setBlocksDirty(x, y - 1, z, x, y, z);
  playDoorToggleSound(world, x, y, z);
}
std::optional<net::minecraft::HitResult> DoorBlock::raycast(
    World* world, int x, int y, int z, net::minecraft::Vec3d startPos, net::minecraft::Vec3d endPos) const {
  if(world == nullptr) {
    return std::nullopt;
  }
  const_cast<DoorBlock*>(this)->updateBoundingBox(world, x, y, z);
  return Block::raycast(world, x, y, z, startPos, endPos);
}
void DoorBlock::neighborUpdate(World* world, int x, int y, int z, int blockId) {
  if(world == nullptr) {
    return;
  }
  const int meta = world->getBlockMeta(x, y, z);
  if((meta & 8) != 0) {
    if(world->getBlockId(x, y - 1, z) != id) {
      world->setBlock(x, y, z, 0);
    } else if(blockId > 0 && blockId < Block::BLOCK_COUNT &&
              Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr &&
              Block::BLOCKS[static_cast<std::size_t>(blockId)]->canEmitRedstonePower()) {
      neighborUpdate(world, x, y - 1, z, blockId);
    }
    return;
  }
  bool removed = false;
  if(world->getBlockId(x, y + 1, z) != id) {
    world->setBlock(x, y, z, 0);
    removed = true;
  }
  if(!world->shouldSuffocate(x, y - 1, z)) {
    world->setBlock(x, y, z, 0);
    removed = true;
    if(world->getBlockId(x, y + 1, z) == id) {
      world->setBlock(x, y + 1, z, 0);
    }
  }
  if(removed) {
    if(!world->isRemote()) {
      dropStacks(world, x, y, z, meta);
    }
    return;
  }
  if(blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr &&
     Block::BLOCKS[static_cast<std::size_t>(blockId)]->canEmitRedstonePower()) {
    const bool powered = world->isEmittingRedstonePower(x, y, z) || world->isEmittingRedstonePower(x, y + 1, z);
    setOpen(world, x, y, z, powered);
  }
}
void DoorBlock::registerClass() {
  namespace mat = material;
  Block::DOOR = (new DoorBlock(64, mat::Material::WOOD))
                    ->setHardness(3.0f)
                    ->setSoundGroup(&kWoodSound)
                    ->setTranslationKey("doorWood")
                    ->disableTrackingStatistics()
                    ->ignoreMetaUpdates();
  Block::IRON_DOOR = (new DoorBlock(kBlockId, mat::Material::METAL))
                         ->setHardness(5.0f)
                         ->setSoundGroup(&kMetalSound)
                         ->setTranslationKey("doorIron")
                         ->disableTrackingStatistics()
                         ->ignoreMetaUpdates();
}
MC_REGISTER_BLOCK(DoorBlock)
} // namespace net::minecraft::block
