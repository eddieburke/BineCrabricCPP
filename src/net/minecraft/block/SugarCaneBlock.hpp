#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/sugar_cane.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"
namespace net::minecraft::block {
// Registered in Block.cpp.
class SugarCaneBlock : public Block {
public:
  using Block::canPlaceAt;
  SugarCaneBlock(int id, int textureId) : Block(id, textureId, material::Material::PLANT) {
    this->textureId = textureId;
    const float f = 0.375f;
    setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 1.0f, 0.5f + f);
    setTickRandomly(true);
  }
  void onTick(World* world, int x, int y, int z, JavaRandom& /*random*/) override {
    if(world == nullptr || world->isRemote()) {
      return;
    }
    if(!world->isAir(x, y + 1, z)) {
      return;
    }
    int height = 1;
    while(world->getBlockId(x, y - height, z) == id) {
      ++height;
    }
    if(height >= 3) {
      return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    if(meta == 15) {
      world->setBlock(x, y + 1, z, id);
      world->setBlockMeta(x, y, z, 0);
    } else {
      world->setBlockMeta(x, y, z, meta + 1);
    }
  }
  [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const {
    const int belowId = world->getBlockId(x, y - 1, z);
    if(belowId == id) {
      return true;
    }
    if(belowId != Block::GRASS_BLOCK->id && belowId != Block::DIRT->id) {
      return false;
    }
    return &world->getMaterial(x - 1, y - 1, z) == &material::Material::WATER ||
           &world->getMaterial(x + 1, y - 1, z) == &material::Material::WATER ||
           &world->getMaterial(x, y - 1, z - 1) == &material::Material::WATER ||
           &world->getMaterial(x, y - 1, z + 1) == &material::Material::WATER;
  }
  void neighborUpdate(World* world, int x, int y, int z, int /*id*/) override {
    if(world == nullptr || world->isRemote()) {
      return;
    }
    breakIfCannotGrow(world, x, y, z);
  }
  [[nodiscard]] bool canGrow(World* world, int x, int y, int z) const override {
    return canPlaceAt(world, x, y, z);
  }
  [[nodiscard]] bool isOpaque() const override {
    return false;
  }
  [[nodiscard]] bool isFullCube() const override {
    return false;
  }
  [[nodiscard]] int getRenderType() const override {
    return 1;
  }
  [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override {
    return Item::byRawId(82) != nullptr ? Item::byRawId(82)->id : 338;
  }
  [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/, int /*x*/, int /*y*/,
                                                                     int /*z*/) const override {
    return std::nullopt;
  }

protected:
  void breakIfCannotGrow(World* world, int x, int y, int z) {
    if(world == nullptr || world->isRemote()) {
      return;
    }
    if(!canGrow(world, x, y, z)) {
      dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
      world->setBlock(x, y, z, 0);
    }
  }
};
} // namespace net::minecraft::block
