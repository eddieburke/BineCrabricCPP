#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
namespace net::minecraft {
// Faithful 1:1 port of net.minecraft.world.gen.feature.PumpkinPatchFeature.
class PumpkinPatchFeature : public Feature {
 public:
 bool generate(World* world, JavaRandom& random, int x, int y, int z) override {
  for(int i = 0; i < 64; ++i) {
   const int nx = x + random.nextInt(8) - random.nextInt(8);
   const int ny = y + random.nextInt(4) - random.nextInt(4);
   const int nz = z + random.nextInt(8) - random.nextInt(8);
   if(!world->isAir(nx, ny, nz) || world->getBlockId(nx, ny - 1, nz) != Block::GRASS_BLOCK->id ||
      !Block::PUMPKIN->canPlaceAt(world, nx, ny, nz)) {
    continue;
   }
   world->setBlockWithoutNotifyingNeighbors(
       nx, ny, nz, Block::PUMPKIN->id, static_cast<std::uint8_t>(random.nextInt(4)));
  }
  return true;
 }
};
} // namespace net::minecraft
