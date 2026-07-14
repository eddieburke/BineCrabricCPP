#include "net/minecraft/block/RedstoneTorchBlock.hpp"
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
namespace {
net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
}
namespace net::minecraft::block {
namespace {
struct RedstoneTorchBurnoutEntry {
  int x = 0;
  int y = 0;
  int z = 0;
  std::uint64_t time = 0;
};
std::vector<RedstoneTorchBurnoutEntry> g_burnoutEntries;
} // namespace
RedstoneTorchBlock::RedstoneTorchBlock(int id, int textureId, bool litIn) : TorchBlock(id, textureId) {
  lit = litIn;
  setTickRandomly(true);
}
int RedstoneTorchBlock::getTexture(int side, int meta) const {
  if(side == 1 && Block::REDSTONE_WIRE != nullptr) {
    return Block::REDSTONE_WIRE->getTexture(side, meta);
  }
  return TorchBlock::getTexture(side, meta);
}
int RedstoneTorchBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const {
  return Block::LIT_REDSTONE_TORCH != nullptr ? Block::LIT_REDSTONE_TORCH->id : 76;
}
bool RedstoneTorchBlock::isBurnedOut(World* world, int x, int y, int z, bool addNew) {
  if(addNew && world != nullptr) {
    g_burnoutEntries.push_back(RedstoneTorchBurnoutEntry{x, y, z, world->getTime()});
  }
  int count = 0;
  for(const RedstoneTorchBurnoutEntry& entry : g_burnoutEntries) {
    if(entry.x != x || entry.y != y || entry.z != z || ++count < 8) {
      continue;
    }
    return true;
  }
  return false;
}
bool RedstoneTorchBlock::shouldUnpower(World* world, int x, int y, int z) const {
  if(world == nullptr) {
    return false;
  }
  const int meta = world->getBlockMeta(x, y, z);
  if(meta == 5 && world->isEmittingRedstonePowerInDirection(x, y - 1, z, 0)) {
    return true;
  }
  if(meta == 3 && world->isEmittingRedstonePowerInDirection(x, y, z - 1, 2)) {
    return true;
  }
  if(meta == 4 && world->isEmittingRedstonePowerInDirection(x, y, z + 1, 3)) {
    return true;
  }
  if(meta == 1 && world->isEmittingRedstonePowerInDirection(x - 1, y, z, 4)) {
    return true;
  }
  return meta == 2 && world->isEmittingRedstonePowerInDirection(x + 1, y, z, 5);
}
void RedstoneTorchBlock::onPlaced(World* world, int x, int y, int z) {
  if(world != nullptr && world->getBlockMeta(x, y, z) == 0) {
    TorchBlock::onPlaced(world, x, y, z);
  }
  if(!lit || world == nullptr) {
    return;
  }
  world->notifyNeighbors(x, y - 1, z, id);
  world->notifyNeighbors(x, y + 1, z, id);
  world->notifyNeighbors(x - 1, y, z, id);
  world->notifyNeighbors(x + 1, y, z, id);
  world->notifyNeighbors(x, y, z - 1, id);
  world->notifyNeighbors(x, y, z + 1, id);
}
void RedstoneTorchBlock::onBreak(World* world, int x, int y, int z) {
  if(!lit || world == nullptr) {
    return;
  }
  world->notifyNeighbors(x, y - 1, z, id);
  world->notifyNeighbors(x, y + 1, z, id);
  world->notifyNeighbors(x - 1, y, z, id);
  world->notifyNeighbors(x + 1, y, z, id);
  world->notifyNeighbors(x, y, z - 1, id);
  world->notifyNeighbors(x, y, z + 1, id);
}
void RedstoneTorchBlock::onTick(World* world, int x, int y, int z, JavaRandom& random) {
  if(world == nullptr) {
    return;
  }
  const bool unpower = shouldUnpower(world, x, y, z);
  while(!g_burnoutEntries.empty() && world->getTime() - g_burnoutEntries.front().time > 100ULL) {
    g_burnoutEntries.erase(g_burnoutEntries.begin());
  }
  if(lit) {
    if(unpower && Block::REDSTONE_TORCH != nullptr) {
      world->setBlock(
          x, y, z, Block::REDSTONE_TORCH->id, static_cast<std::uint8_t>(world->getBlockMeta(x, y, z)));
      if(isBurnedOut(world, x, y, z, true)) {
        world->playSound(static_cast<double>(x) + 0.5,
                         static_cast<double>(y) + 0.5,
                         static_cast<double>(z) + 0.5,
                         "random.fizz",
                         0.5f,
                         2.6f + (world->random().nextFloat() - world->random().nextFloat()) * 0.8f);
        for(int i = 0; i < 5; ++i) {
          world->addParticle("smoke",
                             static_cast<double>(x) + random.nextDouble() * 0.6 + 0.2,
                             static_cast<double>(y) + random.nextDouble() * 0.6 + 0.2,
                             static_cast<double>(z) + random.nextDouble() * 0.6 + 0.2,
                             0.0,
                             0.0,
                             0.0);
        }
      }
    }
  } else if(!unpower && !isBurnedOut(world, x, y, z, false) && Block::LIT_REDSTONE_TORCH != nullptr) {
    world->setBlock(
        x, y, z, Block::LIT_REDSTONE_TORCH->id, static_cast<std::uint8_t>(world->getBlockMeta(x, y, z)));
  }
}
void RedstoneTorchBlock::neighborUpdate(World* world, int x, int y, int z, int id) {
  TorchBlock::neighborUpdate(world, x, y, z, id);
  if(world != nullptr) {
    world->scheduleBlockUpdate(x, y, z, this->id, getTickRate());
  }
}
bool RedstoneTorchBlock::canTransferPowerInDirection(World* world, int x, int y, int z, int direction) const {
  if(direction != 0 || world == nullptr) {
    return false;
  }
  return isEmittingRedstonePowerInDirection(world, x, y, z, direction);
}
void RedstoneTorchBlock::randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) {
  if(!lit || world == nullptr) {
    return;
  }
  const int meta = world->getBlockMeta(x, y, z);
  const double px = static_cast<double>(x) + 0.5 + (random.nextFloat() - 0.5f) * 0.2;
  const double py = static_cast<double>(y) + 0.7 + (random.nextFloat() - 0.5f) * 0.2;
  const double pz = static_cast<double>(z) + 0.5 + (random.nextFloat() - 0.5f) * 0.2;
  constexpr double offsetY = 0.22;
  constexpr double offsetXZ = 0.27;
  if(meta == 1) {
    world->addParticle("reddust", px - offsetXZ, py + offsetY, pz, 0.0, 0.0, 0.0);
  } else if(meta == 2) {
    world->addParticle("reddust", px + offsetXZ, py + offsetY, pz, 0.0, 0.0, 0.0);
  } else if(meta == 3) {
    world->addParticle("reddust", px, py + offsetY, pz - offsetXZ, 0.0, 0.0, 0.0);
  } else if(meta == 4) {
    world->addParticle("reddust", px, py + offsetY, pz + offsetXZ, 0.0, 0.0, 0.0);
  } else {
    world->addParticle("reddust", px, py, pz, 0.0, 0.0, 0.0);
  }
}
bool RedstoneTorchBlock::isEmittingRedstonePowerInDirection(
    const BlockView* blockView, int x, int y, int z, int direction) const {
  if(!lit || blockView == nullptr) {
    return false;
  }
  const int meta = blockView->getBlockMeta(x, y, z);
  if(meta == 5 && direction == 1) {
    return false;
  }
  if(meta == 3 && direction == 3) {
    return false;
  }
  if(meta == 4 && direction == 2) {
    return false;
  }
  if(meta == 1 && direction == 5) {
    return false;
  }
  return meta != 2 || direction != 4;
}
void RedstoneTorchBlock::registerClass() {
  Block::REDSTONE_TORCH = (new RedstoneTorchBlock(75, 115, false))
                              ->setHardness(0.0f)
                              ->setSoundGroup(&kWoodSound)
                              ->setTranslationKey("notGate")
                              ->ignoreMetaUpdates();
  Block::LIT_REDSTONE_TORCH = (new RedstoneTorchBlock(kBlockId, 99, true))
                                  ->setHardness(0.0f)
                                  ->setLuminance(0.5f)
                                  ->setSoundGroup(&kWoodSound)
                                  ->setTranslationKey("notGate")
                                  ->ignoreMetaUpdates();
}
void RedstoneTorchBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Block::LIT_REDSTONE_TORCH),
                                {std::string("X"), std::string("#"), '#', Item::byRawId(24), 'X', Item::byRawId(75)});
}
namespace {
} // namespace
MC_REGISTER_BLOCK(RedstoneTorchBlock)
} // namespace net::minecraft::block
