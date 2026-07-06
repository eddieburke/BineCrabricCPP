#pragma once
#include "net/minecraft/block/PlantBlock.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/SeedsItem.hpp"
#include "net/minecraft/item/food/wheat.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/ports/IEntityWorld.hpp"
namespace net::minecraft::block {
// Registered in Block.cpp.
class CropBlock : public PlantBlock {
public:
  CropBlock(int id, int textureIdIn) : PlantBlock(id, textureIdIn) {
    textureId = textureIdIn;
    setTickRandomly(true);
    constexpr float f = 0.5f;
    setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.25f, 0.5f + f);
  }
  [[nodiscard]] int getRenderType() const override {
    return 6;
  }
  [[nodiscard]] int getTexture(int /*side*/, int meta) const override {
    if(meta < 0) {
      meta = 7;
    }
    return textureId + meta;
  }

protected:
  [[nodiscard]] bool canPlantOnTop(int belowId) const override {
    return Block::FARMLAND != nullptr && belowId == Block::FARMLAND->id;
  }
  void onTick(World* world, int x, int y, int z, JavaRandom& random) override {
    PlantBlock::onTick(world, x, y, z, random);
    if(world == nullptr) {
      return;
    }
    if(world->getLightLevelAbove(x, y, z) < 9) {
      return;
    }
    int meta = world->getBlockMeta(x, y, z);
    if(meta >= 7) {
      return;
    }
    const float moisture = getAvailableMoisture(world, x, y, z);
    const int growthOdds = static_cast<int>(100.0f / moisture);
    if(growthOdds <= 0 || random.nextInt(growthOdds) != 0) {
      return;
    }
    world->setBlockMeta(x, y, z, ++meta);
  }
  void dropStacks(World* world, int x, int y, int z, int meta, float luck) override {
    Block::dropStacks(world, x, y, z, meta, luck);
    if(world == nullptr || world->isRemote() || Item::byRawId(39) == nullptr) {
      return;
    }
    for(int i = 0; i < 3; ++i) {
      if(world->random().nextInt(15) > meta) {
        continue;
      }
      constexpr float spread = 0.7f;
      const float offsetX = world->random().nextFloat() * spread + (1.0f - spread) * 0.5f;
      const float offsetY = world->random().nextFloat() * spread + (1.0f - spread) * 0.5f;
      const float offsetZ = world->random().nextFloat() * spread + (1.0f - spread) * 0.5f;
      auto* itemEntity = new ItemEntity(world, static_cast<double>(x) + offsetX, static_cast<double>(y) + offsetY,
                                        static_cast<double>(z) + offsetZ, ItemStack(Item::byRawId(39)->id, 1));
      itemEntity->pickupDelay = 10;
      world->spawnEntity(itemEntity);
    }
  }
  [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& /*random*/) const override {
    if(blockMeta == 7 && Item::byRawId(40) != nullptr) {
      return Item::byRawId(40)->id;
    }
    return -1;
  }
  [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override {
    return 1;
  }
  void applyFullGrowth(World* world, int x, int y, int z) {
    if(world != nullptr) {
      world->setBlockMeta(x, y, z, 7);
    }
  }

private:
  [[nodiscard]] float getAvailableMoisture(World* world, int x, int y, int z) const {
    float moisture = 1.0f;
    const int north = world->getBlockId(x, y, z - 1);
    const int south = world->getBlockId(x, y, z + 1);
    const int west = world->getBlockId(x - 1, y, z);
    const int east = world->getBlockId(x + 1, y, z);
    const int northWest = world->getBlockId(x - 1, y, z - 1);
    const int northEast = world->getBlockId(x + 1, y, z - 1);
    const int southEast = world->getBlockId(x + 1, y, z + 1);
    const int southWest = world->getBlockId(x - 1, y, z + 1);
    const bool hasWestCrop = west == id || east == id;
    const bool hasNorthCrop = north == id || south == id;
    const bool hasDiagonalCrop = northWest == id || northEast == id || southEast == id || southWest == id;
    for(int ix = x - 1; ix <= x + 1; ++ix) {
      for(int iz = z - 1; iz <= z + 1; ++iz) {
        const int belowId = world->getBlockId(ix, y - 1, iz);
        float localMoisture = 0.0f;
        if(Block::FARMLAND != nullptr && belowId == Block::FARMLAND->id) {
          localMoisture = 1.0f;
          if(world->getBlockMeta(ix, y - 1, iz) > 0) {
            localMoisture = 3.0f;
          }
        }
        if(ix != x || iz != z) {
          localMoisture /= 4.0f;
        }
        moisture += localMoisture;
      }
    }
    if(hasDiagonalCrop || (hasWestCrop && hasNorthCrop)) {
      moisture /= 2.0f;
    }
    return moisture;
  }
};
} // namespace net::minecraft::block
