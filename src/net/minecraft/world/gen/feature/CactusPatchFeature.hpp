#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
namespace net::minecraft {
// Faithful 1:1 port of net.minecraft.world.gen.feature.CactusPatchFeature.
class CactusPatchFeature : public Feature {
 public:
 bool generate(World* world, JavaRandom& random, int x, int y, int z) override {
  for(int i = 0; i < 10; ++i) {
   const int nx = x + random.nextInt(8) - random.nextInt(8);
   const int ny = y + random.nextInt(4) - random.nextInt(4);
   const int nz = z + random.nextInt(8) - random.nextInt(8);
   if(!world->isAir(nx, ny, nz)) {
    continue;
   }
   const int height = 1 + random.nextInt(random.nextInt(3) + 1);
   for(int j = 0; j < height; ++j) {
    if(!Block::CACTUS->canGrow(world, nx, ny + j, nz)) {
     continue;
    }
    world->setBlockWithoutNotifyingNeighbors(nx, ny + j, nz, Block::CACTUS->id);
   }
  }
  return true;
 }
};
} // namespace net::minecraft
