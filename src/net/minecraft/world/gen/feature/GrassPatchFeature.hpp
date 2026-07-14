#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
namespace net::minecraft {
// Faithful 1:1 port of net.minecraft.world.gen.feature.GrassPatchFeature.
class GrassPatchFeature : public Feature {
public:
  GrassPatchFeature(int blockId, int meta) : blockId_(blockId), meta_(meta) {
  }
  bool generate(World* world, JavaRandom& random, int x, int y, int z) override {
    if(world == nullptr || blockId_ <= 0 || blockId_ >= Block::BLOCK_COUNT) {
      return false;
    }
    Block* plantBlock = Block::BLOCKS[static_cast<std::size_t>(blockId_)];
    if(plantBlock == nullptr) {
      return false;
    }
    const int leavesId = Block::LEAVES != nullptr ? Block::LEAVES->id : 18;
    int n = 0;
    while(((n = world->getBlockId(x, y, z)) == 0 || n == leavesId) && y > 0) {
      --y;
    }
    for(int i = 0; i < 128; ++i) {
      const int nx = x + random.nextInt(8) - random.nextInt(8);
      const int ny = y + random.nextInt(4) - random.nextInt(4);
      const int nz = z + random.nextInt(8) - random.nextInt(8);
      if(!world->isAir(nx, ny, nz) || !plantBlock->canGrow(world, nx, ny, nz)) {
        continue;
      }
      world->setBlockWithoutNotifyingNeighbors(nx, ny, nz, blockId_, meta_);
    }
    return true;
  }

private:
  int blockId_ = 0;
  int meta_ = 0;
};
} // namespace net::minecraft
