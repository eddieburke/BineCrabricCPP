#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/ChestBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/ChestBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/Item.hpp"
namespace {
net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
}
namespace net::minecraft::block {
using net::minecraft::entity::ItemEntity;
int ChestBlock::getTextureId(const BlockView* blockView, int x, int y, int z, int side) const {
  if(blockView == nullptr) {
    return getTexture(side);
  }
  if(side == 1 || side == 0) {
    return textureId - 1;
  }
  const int northId = blockView->getBlockId(x, y, z - 1);
  const int southId = blockView->getBlockId(x, y, z + 1);
  const int westId = blockView->getBlockId(x - 1, y, z);
  const int eastId = blockView->getBlockId(x + 1, y, z);
  if(northId == id || southId == id) {
    if(side == 2 || side == 3) {
      return textureId;
    }
    int offset = northId == id ? -1 : 0;
    const int cornerWestId = blockView->getBlockId(x - 1, y, northId == id ? z - 1 : z + 1);
    const int cornerEastId = blockView->getBlockId(x + 1, y, northId == id ? z - 1 : z + 1);
    if(side == 4) {
      offset = -1 - offset;
    }
    int frontSide = 5;
    if((Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)] ||
        Block::BLOCKS_OPAQUE[static_cast<std::size_t>(cornerWestId)]) &&
       !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)] &&
       !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(cornerEastId)]) {
      frontSide = 5;
    }
    if((Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)] ||
        Block::BLOCKS_OPAQUE[static_cast<std::size_t>(cornerEastId)]) &&
       !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)] &&
       !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(cornerWestId)]) {
      frontSide = 4;
    }
    return (side == frontSide ? textureId + 16 : textureId + 32) + offset;
  }
  if(westId == id || eastId == id) {
    if(side == 4 || side == 5) {
      return textureId;
    }
    int offset = westId == id ? -1 : 0;
    const int cornerNorthId = blockView->getBlockId(westId == id ? x - 1 : x + 1, y, z - 1);
    const int cornerSouthId = blockView->getBlockId(westId == id ? x - 1 : x + 1, y, z + 1);
    if(side == 3) {
      offset = -1 - offset;
    }
    int frontSide = 3;
    if((Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)] ||
        Block::BLOCKS_OPAQUE[static_cast<std::size_t>(cornerNorthId)]) &&
       !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)] &&
       !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(cornerSouthId)]) {
      frontSide = 3;
    }
    if((Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)] ||
        Block::BLOCKS_OPAQUE[static_cast<std::size_t>(cornerSouthId)]) &&
       !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)] &&
       !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(cornerNorthId)]) {
      frontSide = 2;
    }
    return (side == frontSide ? textureId + 16 : textureId + 32) + offset;
  }
  int frontSide = 3;
  if(Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)] &&
     !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)]) {
    frontSide = 3;
  }
  if(Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)] &&
     !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)]) {
    frontSide = 2;
  }
  if(Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)] &&
     !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)]) {
    frontSide = 5;
  }
  if(Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)] &&
     !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)]) {
    frontSide = 4;
  }
  return side == frontSide ? textureId + 1 : textureId;
}
bool ChestBlock::hasNeighbor(World* world, int x, int y, int z) const {
  if(world == nullptr || world->getBlockId(x, y, z) != id) {
    return false;
  }
  if(world->getBlockId(x - 1, y, z) == id) {
    return true;
  }
  if(world->getBlockId(x + 1, y, z) == id) {
    return true;
  }
  if(world->getBlockId(x, y, z - 1) == id) {
    return true;
  }
  return world->getBlockId(x, y, z + 1) == id;
}
bool ChestBlock::canPlaceAt(World* world, int x, int y, int z) const {
  if(world == nullptr) {
    return false;
  }
  int adjacentChests = 0;
  if(world->getBlockId(x - 1, y, z) == id) {
    ++adjacentChests;
  }
  if(world->getBlockId(x + 1, y, z) == id) {
    ++adjacentChests;
  }
  if(world->getBlockId(x, y, z - 1) == id) {
    ++adjacentChests;
  }
  if(world->getBlockId(x, y, z + 1) == id) {
    ++adjacentChests;
  }
  if(adjacentChests > 1) {
    return false;
  }
  if(hasNeighbor(world, x - 1, y, z)) {
    return false;
  }
  if(hasNeighbor(world, x + 1, y, z)) {
    return false;
  }
  if(hasNeighbor(world, x, y, z - 1)) {
    return false;
  }
  return !hasNeighbor(world, x, y, z + 1);
}
bool ChestBlock::canPlaceAt(World* world, int x, int y, int z, int /*side*/) const {
  return canPlaceAt(world, x, y, z);
}
bool ChestBlock::onUse(World* world, int x, int y, int z, ::net::minecraft::PlayerEntity* player) {
  if(world == nullptr || player == nullptr) {
    return true;
  }
  auto* chest = dynamic_cast<entity::ChestBlockEntity*>(world->getBlockEntity(x, y, z));
  if(chest == nullptr) {
    return true;
  }
  if(world->shouldSuffocate(x, y + 1, z)) {
    return true;
  }
  if(world->getBlockId(x - 1, y, z) == id && world->shouldSuffocate(x - 1, y + 1, z)) {
    return true;
  }
  if(world->getBlockId(x + 1, y, z) == id && world->shouldSuffocate(x + 1, y + 1, z)) {
    return true;
  }
  if(world->getBlockId(x, y, z - 1) == id && world->shouldSuffocate(x, y + 1, z - 1)) {
    return true;
  }
  if(world->getBlockId(x, y, z + 1) == id && world->shouldSuffocate(x, y + 1, z + 1)) {
    return true;
  }
  if(world->isRemote()) {
    return true;
  }
  player->openChestScreen(x, y, z);
  return true;
}
void ChestBlock::onBreak(World* world, int x, int y, int z) {
  if(world == nullptr) {
    BlockWithEntity::onBreak(world, x, y, z);
    return;
  }
  auto* chest = dynamic_cast<entity::ChestBlockEntity*>(world->getBlockEntity(x, y, z));
  if(chest != nullptr) {
    for(std::size_t slot = 0; slot < chest->size(); ++slot) {
      ItemStack stack = chest->getStack(slot);
      if(stack.empty()) {
        continue;
      }
      const float offsetX = random_.nextFloat() * 0.8f + 0.1f;
      const float offsetY = random_.nextFloat() * 0.8f + 0.1f;
      const float offsetZ = random_.nextFloat() * 0.8f + 0.1f;
      while(stack.count > 0) {
        int dropCount = random_.nextInt(21) + 10;
        if(dropCount > stack.count) {
          dropCount = stack.count;
        }
        stack.count -= dropCount;
        auto* itemEntity = new ItemEntity(world, static_cast<double>(x) + static_cast<double>(offsetX),
                                          static_cast<double>(y) + static_cast<double>(offsetY),
                                          static_cast<double>(z) + static_cast<double>(offsetZ),
                                          ItemStack(stack.itemId, dropCount, stack.damage));
        constexpr float spread = 0.05f;
        itemEntity->velocityX = random_.nextGaussian() * spread;
        itemEntity->velocityY = random_.nextGaussian() * spread + 0.2f;
        itemEntity->velocityZ = random_.nextGaussian() * spread;
        world->spawnEntity(itemEntity);
      }
    }
  }
  BlockWithEntity::onBreak(world, x, y, z);
}
void ChestBlock::registerClass() {
  Block::CHEST = (new ChestBlock(kBlockId))
                     ->setHardness(2.5f)
                     ->setSoundGroup(&kWoodSound)
                     ->setTranslationKey("chest")
                     ->ignoreMetaUpdates();
}
void ChestBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Block::CHEST),
                                {std::string("###"), std::string("# #"), std::string("###"), '#', Block::PLANKS});
}
MC_REGISTER_BLOCK(ChestBlock)
} // namespace net::minecraft::block
