#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
namespace net::minecraft {
// Faithful 1:1 port of net.minecraft.world.gen.feature.SugarCanePatchFeature.
class SugarCanePatchFeature : public Feature {
 public:
 bool generate(World* world, JavaRandom& random, int x, int y, int z) override {
  for(int i = 0; i < 20; ++i) {
   const int nx = x + random.nextInt(4) - random.nextInt(4);
   const int ny = y;
   const int nz = z + random.nextInt(4) - random.nextInt(4);
   if(!world->isAir(nx, ny, nz) ||
      (&world->getMaterial(nx - 1, ny - 1, nz) != &block::material::Material::WATER &&
       &world->getMaterial(nx + 1, ny - 1, nz) != &block::material::Material::WATER &&
       &world->getMaterial(nx, ny - 1, nz - 1) != &block::material::Material::WATER &&
       &world->getMaterial(nx, ny - 1, nz + 1) != &block::material::Material::WATER)) {
    continue;
   }
   const int height = 2 + random.nextInt(random.nextInt(3) + 1);
   for(int j = 0; j < height; ++j) {
    if(!Block::SUGAR_CANE->canGrow(world, nx, ny + j, nz)) {
     continue;
    }
    world->setBlockWithoutNotifyingNeighbors(nx, ny + j, nz, Block::SUGAR_CANE->id);
   }
  }
  return true;
 }
};
} // namespace net::minecraft
