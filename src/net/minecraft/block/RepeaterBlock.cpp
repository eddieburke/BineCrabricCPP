#include "net/minecraft/block/RepeaterBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
namespace {
net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
}
namespace net::minecraft::block {
namespace {
constexpr int kDelay[4] = {1, 2, 3, 4};
}
RepeaterBlock::RepeaterBlock(int id, bool litIn) : Block(id, 6, material::Material::PISTON_BREAKABLE) {
  lit = litIn;
  setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
}
int RepeaterBlock::getTexture(int side, int /*meta*/) const {
  if(side == 0) {
    return lit ? 99 : 115;
  }
  if(side == 1) {
    return lit ? 147 : 131;
  }
  return 5;
}
bool RepeaterBlock::isSideVisibleForBounds(const BlockView* /*blockView*/,
                                           int /*x*/,
                                           int /*y*/,
                                           int /*z*/,
                                           int side,
                                           const net::minecraft::Box& /*bounds*/) const {
  return side != 0 && side != 1;
}
bool RepeaterBlock::canPlaceAt(World* world, int x, int y, int z) const {
  return world != nullptr && world->shouldSuffocate(x, y - 1, z) && Block::canPlaceAt(world, x, y, z);
}
bool RepeaterBlock::canGrow(World* world, int x, int y, int z) const {
  return world != nullptr && world->shouldSuffocate(x, y - 1, z) && Block::canGrow(world, x, y, z);
}
int RepeaterBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const {
  return Item::byRawId(100) != nullptr ? Item::byRawId(100)->id : 356;
}
bool RepeaterBlock::isPowered(World* world, int x, int y, int z, int meta) const {
  if(world == nullptr) {
    return false;
  }
  const int facing = meta & 3;
  switch(facing) {
  case 0:
    return world->isEmittingRedstonePowerInDirection(x, y, z + 1, 3) ||
           (Block::REDSTONE_WIRE != nullptr && world->getBlockId(x, y, z + 1) == Block::REDSTONE_WIRE->id &&
            world->getBlockMeta(x, y, z + 1) > 0);
  case 2:
    return world->isEmittingRedstonePowerInDirection(x, y, z - 1, 2) ||
           (Block::REDSTONE_WIRE != nullptr && world->getBlockId(x, y, z - 1) == Block::REDSTONE_WIRE->id &&
            world->getBlockMeta(x, y, z - 1) > 0);
  case 3:
    return world->isEmittingRedstonePowerInDirection(x + 1, y, z, 5) ||
           (Block::REDSTONE_WIRE != nullptr && world->getBlockId(x + 1, y, z) == Block::REDSTONE_WIRE->id &&
            world->getBlockMeta(x + 1, y, z) > 0);
  case 1:
    return world->isEmittingRedstonePowerInDirection(x - 1, y, z, 4) ||
           (Block::REDSTONE_WIRE != nullptr && world->getBlockId(x - 1, y, z) == Block::REDSTONE_WIRE->id &&
            world->getBlockMeta(x - 1, y, z) > 0);
  default:
    return false;
  }
}
void RepeaterBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/) {
  if(world == nullptr) {
    return;
  }
  const int meta = world->getBlockMeta(x, y, z);
  const bool powered = isPowered(world, x, y, z, meta);
  if(lit && !powered && Block::REPEATER != nullptr) {
    world->setBlock(x, y, z, Block::REPEATER->id, static_cast<std::uint8_t>(meta));
  } else if(!lit && Block::POWERED_REPEATER != nullptr) {
    world->setBlock(x, y, z, Block::POWERED_REPEATER->id, static_cast<std::uint8_t>(meta));
    if(!powered) {
      const int delayIndex = (meta & 0xC) >> 2;
      world->scheduleBlockUpdate(x, y, z, Block::POWERED_REPEATER->id, kDelay[delayIndex] * 2);
    }
  }
}
void RepeaterBlock::neighborUpdate(World* world, int x, int y, int z, int /*id*/) {
  if(world == nullptr) {
    return;
  }
  if(!canGrow(world, x, y, z)) {
    dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
    world->setBlock(x, y, z, 0);
    return;
  }
  const int meta = world->getBlockMeta(x, y, z);
  const bool powered = isPowered(world, x, y, z, meta);
  const int delayIndex = (meta & 0xC) >> 2;
  if((lit && !powered) || (!lit && powered)) {
    world->scheduleBlockUpdate(x, y, z, this->id, kDelay[delayIndex] * 2);
  }
}
bool RepeaterBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* /*player*/) {
  if(world == nullptr) {
    return false;
  }
  const int meta = world->getBlockMeta(x, y, z);
  int delayIndex = ((meta & 0xC) >> 2) + 1;
  delayIndex = (delayIndex << 2) & 0xC;
  world->setBlockMeta(x, y, z, delayIndex | (meta & 3));
  return true;
}
void RepeaterBlock::onPlaced(World* world, int x, int y, int z) {
  if(world == nullptr) {
    return;
  }
  world->notifyNeighbors(x + 1, y, z, id);
  world->notifyNeighbors(x - 1, y, z, id);
  world->notifyNeighbors(x, y, z + 1, id);
  world->notifyNeighbors(x, y, z - 1, id);
  world->notifyNeighbors(x, y - 1, z, id);
  world->notifyNeighbors(x, y + 1, z, id);
}
void RepeaterBlock::onPlaced(World* world, int x, int y, int z, net::minecraft::PlayerEntity* placer) {
  if(world == nullptr || placer == nullptr) {
    return;
  }
  const int facing = ((MathHelper::floor(static_cast<double>(placer->yaw * 4.0f / 360.0f) + 0.5) & 3) + 2) % 4;
  world->setBlockMeta(x, y, z, facing);
  if(isPowered(world, x, y, z, facing)) {
    world->scheduleBlockUpdate(x, y, z, id, 1);
  }
}
bool RepeaterBlock::canTransferPowerInDirection(World* world, int x, int y, int z, int direction) const {
  return isEmittingRedstonePowerInDirection(world, x, y, z, direction);
}
bool RepeaterBlock::isEmittingRedstonePowerInDirection(
    const BlockView* blockView, int x, int y, int z, int direction) const {
  if(!lit || blockView == nullptr) {
    return false;
  }
  const int facing = blockView->getBlockMeta(x, y, z) & 3;
  if(facing == 0 && direction == 3) {
    return true;
  }
  if(facing == 1 && direction == 4) {
    return true;
  }
  if(facing == 2 && direction == 2) {
    return true;
  }
  return facing == 3 && direction == 5;
}
void RepeaterBlock::randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) {
  if(!lit || world == nullptr) {
    return;
  }
  const int meta = world->getBlockMeta(x, y, z);
  double px = static_cast<double>(x) + 0.5 + (random.nextFloat() - 0.5f) * 0.2;
  double py = static_cast<double>(y) + 0.4 + (random.nextFloat() - 0.5f) * 0.2;
  double pz = static_cast<double>(z) + 0.5 + (random.nextFloat() - 0.5f) * 0.2;
  double offsetX = 0.0;
  double offsetZ = 0.0;
  if(random.nextInt(2) == 0) {
    switch(meta & 3) {
    case 0:
      offsetZ = -0.3125;
      break;
    case 2:
      offsetZ = 0.3125;
      break;
    case 3:
      offsetX = -0.3125;
      break;
    case 1:
      offsetX = 0.3125;
      break;
    default:
      break;
    }
  } else {
    const int delayIndex = (meta & 0xC) >> 2;
    switch(meta & 3) {
    case 0:
      offsetZ = RENDER_OFFSET[delayIndex];
      break;
    case 2:
      offsetZ = -RENDER_OFFSET[delayIndex];
      break;
    case 3:
      offsetX = RENDER_OFFSET[delayIndex];
      break;
    case 1:
      offsetX = -RENDER_OFFSET[delayIndex];
      break;
    default:
      break;
    }
  }
  world->addParticle("reddust", px + offsetX, py, pz + offsetZ, 0.0, 0.0, 0.0);
}
void RepeaterBlock::registerClass() {
  Block::REPEATER = (new RepeaterBlock(93, false))
                        ->setHardness(0.0f)
                        ->setSoundGroup(&kWoodSound)
                        ->setTranslationKey("diode")
                        ->disableTrackingStatistics()
                        ->ignoreMetaUpdates();
  Block::POWERED_REPEATER = (new RepeaterBlock(kBlockId, true))
                                ->setHardness(0.0f)
                                ->setLuminance(0.625f)
                                ->setSoundGroup(&kWoodSound)
                                ->setTranslationKey("diode")
                                ->disableTrackingStatistics()
                                ->ignoreMetaUpdates();
}
MC_REGISTER_BLOCK(RepeaterBlock)
} // namespace net::minecraft::block
