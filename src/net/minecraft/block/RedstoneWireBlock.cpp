#include "net/minecraft/block/RedstoneWireBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/RedstoneItem.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/math/Facings.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::block {
RedstoneWireBlock::RedstoneWireBlock(int id, int textureId)
    : Block(id, textureId, material::Material::PISTON_BREAKABLE) {
  setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
}
bool RedstoneWireBlock::canPlaceAt(World* world, int x, int y, int z) const {
  return world != nullptr && world->shouldSuffocate(x, y - 1, z);
}
int RedstoneWireBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const {
  return Item::byRawId(75) != nullptr ? Item::byRawId(75)->id : 331;
}
bool RedstoneWireBlock::shouldConnectTo(const BlockView* blockView, int x, int y, int z, int l) {
  if(blockView == nullptr) {
    return false;
  }
  const int blockId = blockView->getBlockId(x, y, z);
  if(Block::REDSTONE_WIRE != nullptr && blockId == Block::REDSTONE_WIRE->id) {
    return true;
  }
  if(blockId == 0) {
    return false;
  }
  if(blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr &&
     Block::BLOCKS[static_cast<std::size_t>(blockId)]->canEmitRedstonePower()) {
    return true;
  }
  if(Block::REPEATER != nullptr && Block::POWERED_REPEATER != nullptr &&
     (blockId == Block::REPEATER->id || blockId == Block::POWERED_REPEATER->id)) {
    const int meta = blockView->getBlockMeta(x, y, z);
    return l == util::math::Facings::OPPOSITE[meta & 3];
  }
  return false;
}
int RedstoneWireBlock::getHighestPowerWire(World* world, int x, int y, int z, int power) const {
  if(world == nullptr || world->getBlockId(x, y, z) != id) {
    return power;
  }
  const int wirePower = world->getBlockMeta(x, y, z);
  return wirePower > power ? wirePower : power;
}
void RedstoneWireBlock::updateNeighborsOfWire(World* world, int x, int y, int z) {
  if(world == nullptr || world->getBlockId(x, y, z) != id) {
    return;
  }
  world->notifyNeighbors(x, y, z, id);
  world->notifyNeighbors(x - 1, y, z, id);
  world->notifyNeighbors(x + 1, y, z, id);
  world->notifyNeighbors(x, y, z - 1, id);
  world->notifyNeighbors(x, y, z + 1, id);
  world->notifyNeighbors(x, y - 1, z, id);
  world->notifyNeighbors(x, y + 1, z, id);
}
void RedstoneWireBlock::doUpdatePower(World* world, int x, int y, int z, int sourceX, int sourceY, int sourceZ) {
  if(world == nullptr) {
    return;
  }
  const int previousPower = world->getBlockMeta(x, y, z);
  int nextPower = 0;
  powered_ = false;
  const bool directPower = world->isEmittingRedstonePower(x, y, z);
  powered_ = true;
  if(directPower) {
    nextPower = 15;
  } else {
    for(int direction = 0; direction < 4; ++direction) {
      int nx = x;
      int nz = z;
      if(direction == 0) {
        --nx;
      } else if(direction == 1) {
        ++nx;
      } else if(direction == 2) {
        --nz;
      } else {
        ++nz;
      }
      if(nx != sourceX || y != sourceY || nz != sourceZ) {
        nextPower = getHighestPowerWire(world, nx, y, nz, nextPower);
      }
      if(world->shouldSuffocate(nx, y, nz) && !world->shouldSuffocate(x, y + 1, z)) {
        if(nx == sourceX && y + 1 == sourceY && nz == sourceZ) {
          continue;
        }
        nextPower = getHighestPowerWire(world, nx, y + 1, nz, nextPower);
      } else if(!world->shouldSuffocate(nx, y, nz) && !(nx == sourceX && y - 1 == sourceY && nz == sourceZ)) {
        nextPower = getHighestPowerWire(world, nx, y - 1, nz, nextPower);
      }
    }
    nextPower = nextPower > 0 ? nextPower - 1 : 0;
  }
  if(previousPower == nextPower) {
    return;
  }
  world->pauseTicking = true;
  world->setBlockMeta(x, y, z, nextPower);
  world->setBlocksDirty(x, y, z, x, y, z);
  world->pauseTicking = false;
  for(int direction = 0; direction < 4; ++direction) {
    int nx = x;
    int nz = z;
    int checkY = y - 1;
    if(direction == 0) {
      --nx;
    } else if(direction == 1) {
      ++nx;
    } else if(direction == 2) {
      --nz;
    } else {
      ++nz;
    }
    if(world->shouldSuffocate(nx, y, nz)) {
      checkY += 2;
    }
    int wirePower = getHighestPowerWire(world, nx, y, nz, -1);
    int currentMeta = world->getBlockMeta(x, y, z);
    if(currentMeta > 0) {
      --currentMeta;
    }
    if(wirePower >= 0 && wirePower != currentMeta) {
      doUpdatePower(world, nx, y, nz, x, y, z);
    }
    wirePower = getHighestPowerWire(world, nx, checkY, nz, -1);
    currentMeta = world->getBlockMeta(x, y, z);
    if(currentMeta > 0) {
      --currentMeta;
    }
    if(wirePower >= 0 && wirePower != currentMeta) {
      doUpdatePower(world, nx, checkY, nz, x, y, z);
    }
  }
  if(previousPower == 0 || nextPower == 0) {
    neighborsToUpdate_.insert(Vec3i{x, y, z});
    neighborsToUpdate_.insert(Vec3i{x - 1, y, z});
    neighborsToUpdate_.insert(Vec3i{x + 1, y, z});
    neighborsToUpdate_.insert(Vec3i{x, y - 1, z});
    neighborsToUpdate_.insert(Vec3i{x, y + 1, z});
    neighborsToUpdate_.insert(Vec3i{x, y, z - 1});
    neighborsToUpdate_.insert(Vec3i{x, y, z + 1});
  }
}
void RedstoneWireBlock::updatePower(World* world, int x, int y, int z) {
  if(world == nullptr) {
    return;
  }
  doUpdatePower(world, x, y, z, x, y, z);
  const std::vector<Vec3i> pending(neighborsToUpdate_.begin(), neighborsToUpdate_.end());
  neighborsToUpdate_.clear();
  for(const Vec3i& pos : pending) {
    world->notifyNeighbors(pos.x, pos.y, pos.z, id);
  }
}
void RedstoneWireBlock::onPlaced(World* world, int x, int y, int z) {
  Block::onPlaced(world, x, y, z);
  if(world == nullptr || world->isRemote()) {
    return;
  }
  updatePower(world, x, y, z);
  world->notifyNeighbors(x, y + 1, z, id);
  world->notifyNeighbors(x, y - 1, z, id);
  updateNeighborsOfWire(world, x - 1, y, z);
  updateNeighborsOfWire(world, x + 1, y, z);
  updateNeighborsOfWire(world, x, y, z - 1);
  updateNeighborsOfWire(world, x, y, z + 1);
  if(world->shouldSuffocate(x - 1, y, z)) {
    updateNeighborsOfWire(world, x - 1, y + 1, z);
  } else {
    updateNeighborsOfWire(world, x - 1, y - 1, z);
  }
  if(world->shouldSuffocate(x + 1, y, z)) {
    updateNeighborsOfWire(world, x + 1, y + 1, z);
  } else {
    updateNeighborsOfWire(world, x + 1, y - 1, z);
  }
  if(world->shouldSuffocate(x, y, z - 1)) {
    updateNeighborsOfWire(world, x, y + 1, z - 1);
  } else {
    updateNeighborsOfWire(world, x, y - 1, z - 1);
  }
  if(world->shouldSuffocate(x, y, z + 1)) {
    updateNeighborsOfWire(world, x, y + 1, z + 1);
  } else {
    updateNeighborsOfWire(world, x, y - 1, z + 1);
  }
}
void RedstoneWireBlock::onBreak(World* world, int x, int y, int z) {
  Block::onBreak(world, x, y, z);
  if(world == nullptr || world->isRemote()) {
    return;
  }
  world->notifyNeighbors(x, y + 1, z, id);
  world->notifyNeighbors(x, y - 1, z, id);
  updatePower(world, x, y, z);
  updateNeighborsOfWire(world, x - 1, y, z);
  updateNeighborsOfWire(world, x + 1, y, z);
  updateNeighborsOfWire(world, x, y, z - 1);
  updateNeighborsOfWire(world, x, y, z + 1);
  if(world->shouldSuffocate(x - 1, y, z)) {
    updateNeighborsOfWire(world, x - 1, y + 1, z);
  } else {
    updateNeighborsOfWire(world, x - 1, y - 1, z);
  }
  if(world->shouldSuffocate(x + 1, y, z)) {
    updateNeighborsOfWire(world, x + 1, y + 1, z);
  } else {
    updateNeighborsOfWire(world, x + 1, y - 1, z);
  }
  if(world->shouldSuffocate(x, y, z - 1)) {
    updateNeighborsOfWire(world, x, y + 1, z - 1);
  } else {
    updateNeighborsOfWire(world, x, y - 1, z - 1);
  }
  if(world->shouldSuffocate(x, y, z + 1)) {
    updateNeighborsOfWire(world, x, y + 1, z + 1);
  } else {
    updateNeighborsOfWire(world, x, y - 1, z + 1);
  }
}
void RedstoneWireBlock::neighborUpdate(World* world, int x, int y, int z, int id) {
  if(world == nullptr || world->isRemote()) {
    return;
  }
  const int meta = world->getBlockMeta(x, y, z);
  if(!canPlaceAt(world, x, y, z)) {
    dropStacks(world, x, y, z, meta);
    world->setBlock(x, y, z, 0);
  } else {
    updatePower(world, x, y, z);
  }
  Block::neighborUpdate(world, x, y, z, id);
}
bool RedstoneWireBlock::canTransferPowerInDirection(World* world, int x, int y, int z, int direction) const {
  if(!powered_ || world == nullptr) {
    return false;
  }
  return isEmittingRedstonePowerInDirection(world, x, y, z, direction);
}
bool RedstoneWireBlock::isEmittingRedstonePowerInDirection(
    const BlockView* blockView, int x, int y, int z, int direction) const {
  if(!powered_ || blockView == nullptr || blockView->getBlockMeta(x, y, z) == 0) {
    return false;
  }
  if(direction == 1) {
    return true;
  }
  const bool west = shouldConnectTo(blockView, x - 1, y, z, 1) ||
                    (!blockView->shouldSuffocate(x - 1, y, z) && shouldConnectTo(blockView, x - 1, y - 1, z, -1));
  const bool east = shouldConnectTo(blockView, x + 1, y, z, 3) ||
                    (!blockView->shouldSuffocate(x + 1, y, z) && shouldConnectTo(blockView, x + 1, y - 1, z, -1));
  const bool north = shouldConnectTo(blockView, x, y, z - 1, 2) ||
                     (!blockView->shouldSuffocate(x, y, z - 1) && shouldConnectTo(blockView, x, y - 1, z - 1, -1));
  const bool south = shouldConnectTo(blockView, x, y, z + 1, 0) ||
                     (!blockView->shouldSuffocate(x, y, z + 1) && shouldConnectTo(blockView, x, y - 1, z + 1, -1));
  bool westConn = west;
  bool eastConn = east;
  bool northConn = north;
  bool southConn = south;
  if(!blockView->shouldSuffocate(x, y + 1, z)) {
    if(blockView->shouldSuffocate(x - 1, y, z) && shouldConnectTo(blockView, x - 1, y + 1, z, -1)) {
      westConn = true;
    }
    if(blockView->shouldSuffocate(x + 1, y, z) && shouldConnectTo(blockView, x + 1, y + 1, z, -1)) {
      eastConn = true;
    }
    if(blockView->shouldSuffocate(x, y, z - 1) && shouldConnectTo(blockView, x, y + 1, z - 1, -1)) {
      northConn = true;
    }
    if(blockView->shouldSuffocate(x, y, z + 1) && shouldConnectTo(blockView, x, y + 1, z + 1, -1)) {
      southConn = true;
    }
  }
  if(!(northConn || eastConn || westConn || southConn) && direction >= 2 && direction <= 5) {
    return true;
  }
  if(direction == 2 && northConn && !westConn && !eastConn) {
    return true;
  }
  if(direction == 3 && southConn && !westConn && !eastConn) {
    return true;
  }
  if(direction == 4 && westConn && !northConn && !southConn) {
    return true;
  }
  return direction == 5 && eastConn && !northConn && !southConn;
}
void RedstoneWireBlock::randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) {
  if(world == nullptr) {
    return;
  }
  const int power = world->getBlockMeta(x, y, z);
  if(power <= 0) {
    return;
  }
  const double px = static_cast<double>(x) + 0.5 + (random.nextFloat() - 0.5f) * 0.2;
  const double py = static_cast<double>(y) + 0.0625;
  const double pz = static_cast<double>(z) + 0.5 + (random.nextFloat() - 0.5f) * 0.2;
  const float strength = static_cast<float>(power) / 15.0f;
  float redIn = strength * 0.6f + 0.4f;
  if(power == 0) {
    redIn = 0.0f;
  }
  float greenIn = strength * strength * 0.7f - 0.5f;
  float blueIn = strength * strength * 0.6f - 0.7f;
  if(greenIn < 0.0f) {
    greenIn = 0.0f;
  }
  if(blueIn < 0.0f) {
    blueIn = 0.0f;
  }
  world->addParticle(
      "reddust", px, py, pz, static_cast<double>(redIn), static_cast<double>(greenIn), static_cast<double>(blueIn));
}
void RedstoneWireBlock::registerClass() {
  Block::REDSTONE_WIRE = (new RedstoneWireBlock(kBlockId, 164))
                             ->setHardness(0.0f)
                             ->setTranslationKey("redstoneDust")
                             ->disableTrackingStatistics()
                             ->ignoreMetaUpdates();
}
MC_REGISTER_BLOCK(RedstoneWireBlock)
} // namespace net::minecraft::block
