#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"
namespace net::minecraft::block {
// Registered in Block.cpp.
class FarmlandBlock : public Block {
 public:
 explicit FarmlandBlock(int id) : Block(id, material::Material::SOIL) {
  textureId = 87;
  setTickRandomly(true);
  setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.9375f, 1.0f);
  setOpacity(255);
 }
 [[nodiscard]] bool isOpaque() const override {
  return false;
 }
 [[nodiscard]] bool isFullCube() const override {
  return false;
 }
 [[nodiscard]] int getTexture(int side, int meta) const override {
  if(side == 1 && meta > 0) {
   return textureId - 1;
  }
  if(side == 1) {
   return textureId;
  }
  return 2;
 }
 [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/,
                                                                    int x,
                                                                    int y,
                                                                    int z) const override {
  return net::minecraft::Box{
      static_cast<double>(x),
      static_cast<double>(y),
      static_cast<double>(z),
      static_cast<double>(x + 1),
      static_cast<double>(y + 1),
      static_cast<double>(z + 1),
  };
 }
 void onTick(World* world, int x, int y, int z, JavaRandom& random) override {
  if(world == nullptr || random.nextInt(5) != 0) {
   return;
  }
  if(isWaterNearby(world, x, y, z) || world->isRaining(x, y + 1, z)) {
   world->setBlockMeta(x, y, z, 7);
   return;
  }
  int meta = world->getBlockMeta(x, y, z);
  if(meta > 0) {
   world->setBlockMeta(x, y, z, meta - 1);
  } else if(!hasCrop(world, x, y, z) && Block::DIRT != nullptr) {
   world->setBlock(x, y, z, Block::DIRT->id);
  }
 }
 void onSteppedOn(World* world, int x, int y, int z, net::minecraft::Entity* /*entity*/) override {
  if(world != nullptr && world->random().nextInt(4) == 0 && Block::DIRT != nullptr) {
   world->setBlock(x, y, z, Block::DIRT->id);
  }
 }
 void neighborUpdate(World* world, int x, int y, int z, int id) override {
  Block::neighborUpdate(world, x, y, z, id);
  if(world == nullptr || Block::DIRT == nullptr) {
   return;
  }
  if(world->getMaterial(x, y + 1, z).isSolid()) {
   world->setBlock(x, y, z, Block::DIRT->id);
  }
 }
 [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& random) const override {
  return Block::DIRT != nullptr ? Block::DIRT->getDroppedItemId(0, random) : 3;
 }

 private:
 [[nodiscard]] bool hasCrop(World* world, int x, int y, int z) const {
  return world != nullptr && Block::WHEAT != nullptr && world->getBlockId(x, y + 1, z) == Block::WHEAT->id;
 }
 [[nodiscard]] bool isWaterNearby(World* world, int x, int y, int z) const {
  for(int ix = x - 4; ix <= x + 4; ++ix) {
   for(int iy = y; iy <= y + 1; ++iy) {
    for(int iz = z - 4; iz <= z + 4; ++iz) {
     if(&world->getMaterial(ix, iy, iz) == &material::Material::WATER) {
      return true;
     }
    }
   }
  }
  return false;
 }
};
} // namespace net::minecraft::block