#include "net/minecraft/block/FlowingLiquidBlock.hpp"
#include "net/minecraft/block/BlockNeighbors.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
namespace net::minecraft::block {
void FlowingLiquidBlock::convertToSource(World* world, int x, int y, int z) {
 if(world == nullptr) {
  return;
 }
 const int meta = world->getBlockMeta(x, y, z);
 world->setBlockWithoutNotifyingNeighbors(x, y, z, id + 1, meta);
 world->setBlocksDirty(x, y, z, x, y, z);
 world->blockUpdateEvent(x, y, z);
}
bool FlowingLiquidBlock::isLiquidBreaking(int blockId) const {
 if(blockId == 0) {
  return false;
 }
 if((Block::DOOR != nullptr && blockId == Block::DOOR->id) ||
    (Block::IRON_DOOR != nullptr && blockId == Block::IRON_DOOR->id) ||
    (Block::SIGN != nullptr && blockId == Block::SIGN->id) ||
    (Block::LADDER != nullptr && blockId == Block::LADDER->id) ||
    (Block::SUGAR_CANE != nullptr && blockId == Block::SUGAR_CANE->id)) {
  return true;
 }
 if(blockId >= Block::BLOCK_COUNT || Block::BLOCKS[static_cast<std::size_t>(blockId)] == nullptr) {
  return false;
 }
 return Block::BLOCKS[static_cast<std::size_t>(blockId)]->material.isSolid();
}
bool FlowingLiquidBlock::isLiquidBreaking(World* world, int x, int y, int z) const {
 if(world == nullptr) {
  return false;
 }
 return isLiquidBreaking(world->getBlockId(x, y, z));
}
bool FlowingLiquidBlock::canSpreadTo(World* world, int x, int y, int z) const {
 if(world == nullptr) {
  return false;
 }
 const int blockId = world->getBlockId(x, y, z);
 if(blockId == 0) {
  return true;
 }
 if(blockId >= Block::BLOCK_COUNT) {
  return false;
 }
 const Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
 if(block == nullptr) {
  return true;
 }
 const material::Material& target = block->material;
 if(&target == &material || &target == &material::Material::LAVA) {
  return false;
 }
 return !isLiquidBreaking(blockId);
}
void FlowingLiquidBlock::spreadTo(World* world, int x, int y, int z, int depth) {
 if(world == nullptr || !canSpreadTo(world, x, y, z)) {
  return;
 }
 const int existingId = world->getBlockId(x, y, z);
 if(existingId > 0) {
  if(&material == &material::Material::LAVA) {
   fizz(world, x, y, z);
  } else if(existingId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(existingId)] != nullptr) {
   Block::BLOCKS[static_cast<std::size_t>(existingId)]->dropStacks(
       world, x, y, z, world->getBlockMeta(x, y, z));
  }
 }
 world->setBlock(x, y, z, id, static_cast<std::uint8_t>(depth));
}
int FlowingLiquidBlock::getDistanceToGap(
    World* world, int x, int y, int z, int distance, int fromDirection) const {
 int best = 1000;
 for(int direction = 0; direction < 4; ++direction) {
  if(direction == (fromDirection ^ 1)) {
   continue;
  }
  const int nx = x + kHorizontalOffsetX[direction];
  const int nz = z + kHorizontalOffsetZ[direction];
  const int blockId = world->getBlockId(nx, y, nz);
  if(blockId != 0) {
   if(isLiquidBreaking(blockId)) {
    continue;
   }
   const Block* block =
       (blockId < Block::BLOCK_COUNT) ? Block::BLOCKS[static_cast<std::size_t>(blockId)] : nullptr;
   if(block != nullptr && &block->material == &material && world->getBlockMeta(nx, y, nz) == 0) {
    continue;
   }
  }
  if(!isLiquidBreaking(world->getBlockId(nx, y - 1, nz))) {
   return distance;
  }
  if(distance >= 4 || best <= distance + 1) {
   continue;
  }
  const int nested = getDistanceToGap(world, nx, y, nz, distance + 1, direction);
  if(nested < best) {
   best = nested;
  }
 }
 return best;
}
std::array<bool, 4> FlowingLiquidBlock::getSpread(World* world, int x, int y, int z) const {
 int distanceToGap[4] = {1000, 1000, 1000, 1000};
 for(int direction = 0; direction < 4; ++direction) {
  const int nx = x + kHorizontalOffsetX[direction];
  const int nz = z + kHorizontalOffsetZ[direction];
  const int blockId = world->getBlockId(nx, y, nz);
  if(blockId != 0) {
   if(isLiquidBreaking(blockId)) {
    continue;
   }
   const Block* block =
       (blockId < Block::BLOCK_COUNT) ? Block::BLOCKS[static_cast<std::size_t>(blockId)] : nullptr;
   if(block != nullptr && &block->material == &material && world->getBlockMeta(nx, y, nz) == 0) {
    continue;
   }
  }
  if(!isLiquidBreaking(world->getBlockId(nx, y - 1, nz))) {
   distanceToGap[direction] = 0;
  } else {
   distanceToGap[direction] = getDistanceToGap(world, nx, y, nz, 1, direction);
  }
 }
 int best = distanceToGap[0];
 for(int direction = 1; direction < 4; ++direction) {
  if(distanceToGap[direction] < best) {
   best = distanceToGap[direction];
  }
 }
 std::array<bool, 4> spread{};
 for(int direction = 0; direction < 4; ++direction) {
  spread[direction] = (distanceToGap[direction] == best);
 }
 return spread;
}
int FlowingLiquidBlock::getLowestDepth(
    World* world, int x, int y, int z, int currentLowest, int& sourceCount) const {
 const int state = getLiquidState(world, x, y, z);
 if(state < 0) {
  return currentLowest;
 }
 if(state == 0) {
  ++sourceCount;
 }
 const int normalized = state >= 8 ? 0 : state;
 return (currentLowest < 0 || normalized < currentLowest) ? normalized : currentLowest;
}
void FlowingLiquidBlock::onPlaced(World* world, int x, int y, int z) {
 LiquidBlock::onPlaced(world, x, y, z);
 if(world != nullptr && world->getBlockId(x, y, z) == id) {
  world->scheduleBlockUpdate(x, y, z, id, getTickRate());
 }
}
void FlowingLiquidBlock::neighborUpdate(World* world, int x, int y, int z, int neighborId) {
 LiquidBlock::neighborUpdate(world, x, y, z, neighborId);
 if(world != nullptr && world->getBlockId(x, y, z) == id) {
  world->scheduleBlockUpdate(x, y, z, id, getTickRate());
 }
}
void FlowingLiquidBlock::onTick(World* world, int x, int y, int z, JavaRandom& random) {
 if(world == nullptr) {
  return;
 }
 int currentState = getLiquidState(world, x, y, z);
 int flowRate = 1;
 if(&material == &material::Material::LAVA && world->dimension != nullptr && !world->dimension->evaporatesWater) {
  flowRate = 2;
 }
 bool shouldConvert = true;
 if(currentState > 0) {
  int lowestDepth = -100;
  int adjacentSources = 0;
  lowestDepth = getLowestDepth(world, x - 1, y, z, lowestDepth, adjacentSources);
  lowestDepth = getLowestDepth(world, x + 1, y, z, lowestDepth, adjacentSources);
  lowestDepth = getLowestDepth(world, x, y, z - 1, lowestDepth, adjacentSources);
  lowestDepth = getLowestDepth(world, x, y, z + 1, lowestDepth, adjacentSources);
  int nextState = lowestDepth + flowRate;
  if(nextState >= 8 || lowestDepth < 0) {
   nextState = -1;
  }
  const int aboveState = getLiquidState(world, x, y + 1, z);
  if(aboveState >= 0) {
   nextState = aboveState >= 8 ? aboveState : aboveState + 8;
  }
  if(adjacentSources >= 2 && &material == &material::Material::WATER) {
   const int belowId = world->getBlockId(x, y - 1, z);
   if(belowId > 0) {
    const Block* belowBlock =
        (belowId < Block::BLOCK_COUNT) ? Block::BLOCKS[static_cast<std::size_t>(belowId)] : nullptr;
    if(belowBlock != nullptr) {
     if(belowBlock->material.isSolid()) {
      nextState = 0;
     } else if(&belowBlock->material == &material && world->getBlockMeta(x, y - 1, z) == 0) {
      nextState = 0;
     }
    }
   }
  }
  int tickDelay = getTickRate();
  if(&material == &material::Material::LAVA && currentState < 8 && nextState < 8 && nextState > currentState &&
     random.nextInt(4) != 0) {
   tickDelay *= 4;
  }
  if(nextState != currentState) {
   currentState = nextState;
   if(currentState < 0) {
    world->setBlock(x, y, z, 0);
    return;
   } else {
    world->setBlockMeta(x, y, z, currentState);
    world->scheduleBlockUpdate(x, y, z, id, tickDelay);
    world->notifyNeighbors(x, y, z, id);
   }
  } else if(shouldConvert) {
   convertToSource(world, x, y, z);
  } else {
   world->scheduleBlockUpdate(x, y, z, id, getTickRate());
  }
 } else {
  convertToSource(world, x, y, z);
 }
 if(currentState < 0) {
  return;
 }
 if(canSpreadTo(world, x, y - 1, z)) {
  if(currentState >= 8) {
   world->setBlock(x, y - 1, z, id, static_cast<std::uint8_t>(currentState));
  } else {
   world->setBlock(x, y - 1, z, id, static_cast<std::uint8_t>(currentState + 8));
  }
  return;
 }
 if(currentState == 0 || isLiquidBreaking(world->getBlockId(x, y - 1, z))) {
  const auto spreadDirs = getSpread(world, x, y, z);
  int spreadDepth = currentState + flowRate;
  if(currentState >= 8) {
   spreadDepth = 1;
  }
  if(spreadDepth < 8) {
   if(spreadDirs[0]) {
    spreadTo(world, x - 1, y, z, spreadDepth);
   }
   if(spreadDirs[1]) {
    spreadTo(world, x + 1, y, z, spreadDepth);
   }
   if(spreadDirs[2]) {
    spreadTo(world, x, y, z - 1, spreadDepth);
   }
   if(spreadDirs[3]) {
    spreadTo(world, x, y, z + 1, spreadDepth);
   }
  }
 }
}
void FlowingLiquidBlock::registerClass() {
 namespace mat = material;
 Block::FLOWING_WATER = (new FlowingLiquidBlock(kBlockId, mat::Material::WATER))
                            ->setHardness(100.0f)
                            ->setOpacity(3)
                            ->setTranslationKey("water")
                            ->disableTrackingStatistics()
                            ->ignoreMetaUpdates();
 Block::FLOWING_LAVA = (new FlowingLiquidBlock(10, mat::Material::LAVA))
                           ->setHardness(0.0f)
                           ->setLuminance(1.0f)
                           ->setOpacity(255)
                           ->setTranslationKey("lava")
                           ->disableTrackingStatistics()
                           ->ignoreMetaUpdates();
}
MC_REGISTER_BLOCK(FlowingLiquidBlock)
} // namespace net::minecraft::block
