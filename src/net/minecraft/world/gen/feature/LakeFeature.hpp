#pragma once
#include <array>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/light/LightType.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
namespace net::minecraft {
// Faithful 1:1 port of net.minecraft.world.gen.feature.LakeFeature.
class LakeFeature : public Feature {
 public:
 explicit LakeFeature(int waterBlockId) : waterBlockId_(waterBlockId) {
 }
 bool generate(World* world, JavaRandom& random, int x, int y, int z) override {
  x -= 8;
  z -= 8;
  while(y > 0 && world->isAir(x, y, z)) {
   --y;
  }
  y -= 4;
  std::array<bool, 2048> filled{};
  const int blobs = random.nextInt(4) + 4;
  for(int b = 0; b < blobs; ++b) {
   const double radiusX = random.nextDouble() * 6.0 + 3.0;
   const double radiusY = random.nextDouble() * 4.0 + 2.0;
   const double radiusZ = random.nextDouble() * 6.0 + 3.0;
   const double centerX = random.nextDouble() * (16.0 - radiusX - 2.0) + 1.0 + radiusX / 2.0;
   const double centerY = random.nextDouble() * (8.0 - radiusY - 4.0) + 2.0 + radiusY / 2.0;
   const double centerZ = random.nextDouble() * (16.0 - radiusZ - 2.0) + 1.0 + radiusZ / 2.0;
   for(int i = 1; i < 15; ++i) {
    for(int j = 1; j < 15; ++j) {
     for(int k = 1; k < 7; ++k) {
      const double normX = (static_cast<double>(i) - centerX) / (radiusX / 2.0);
      const double normY = (static_cast<double>(k) - centerY) / (radiusY / 2.0);
      const double normZ = (static_cast<double>(j) - centerZ) / (radiusZ / 2.0);
      if(!(normX * normX + normY * normY + normZ * normZ < 1.0)) {
       continue;
      }
      filled[static_cast<std::size_t>((i * 16 + j) * 8 + k)] = true;
     }
    }
   }
  }
  for(int lx = 0; lx < 16; ++lx) {
   for(int lz = 0; lz < 16; ++lz) {
    for(int ly = 0; ly < 8; ++ly) {
     const bool edge = !filled[static_cast<std::size_t>((lx * 16 + lz) * 8 + ly)] &&
                       ((lx < 15 && filled[static_cast<std::size_t>(((lx + 1) * 16 + lz) * 8 + ly)]) ||
                        (lx > 0 && filled[static_cast<std::size_t>(((lx - 1) * 16 + lz) * 8 + ly)]) ||
                        (lz < 15 && filled[static_cast<std::size_t>((lx * 16 + (lz + 1)) * 8 + ly)]) ||
                        (lz > 0 && filled[static_cast<std::size_t>((lx * 16 + (lz - 1)) * 8 + ly)]) ||
                        (ly < 7 && filled[static_cast<std::size_t>((lx * 16 + lz) * 8 + (ly + 1))]) ||
                        (ly > 0 && filled[static_cast<std::size_t>((lx * 16 + lz) * 8 + (ly - 1))]));
     if(!edge) {
      continue;
     }
     block::material::Material& material = world->getMaterial(x + lx, y + ly, z + lz);
     if(ly >= 4 && material.isFluid()) {
      return false;
     }
     if(ly >= 4 || material.isSolid() || world->getBlockId(x + lx, y + ly, z + lz) == waterBlockId_) {
      continue;
     }
     return false;
    }
   }
  }
  for(int lx = 0; lx < 16; ++lx) {
   for(int lz = 0; lz < 16; ++lz) {
    for(int ly = 0; ly < 8; ++ly) {
     if(!filled[static_cast<std::size_t>((lx * 16 + lz) * 8 + ly)]) {
      continue;
     }
     world->setBlockWithoutNotifyingNeighbors(x + lx, y + ly, z + lz, ly >= 4 ? 0 : waterBlockId_);
    }
   }
  }
  for(int lx = 0; lx < 16; ++lx) {
   for(int lz = 0; lz < 16; ++lz) {
    for(int ly = 4; ly < 8; ++ly) {
     if(!filled[static_cast<std::size_t>((lx * 16 + lz) * 8 + ly)] ||
        world->getBlockId(x + lx, y + ly - 1, z + lz) != Block::DIRT->id ||
        world->getBrightness(LightType::Sky, x + lx, y + ly, z + lz) <= 0) {
      continue;
     }
     world->setBlockWithoutNotifyingNeighbors(x + lx, y + ly - 1, z + lz, Block::GRASS_BLOCK->id);
    }
   }
  }
  if(&Block::BLOCKS[static_cast<std::size_t>(waterBlockId_)]->material == &block::material::Material::LAVA) {
   for(int lx = 0; lx < 16; ++lx) {
    for(int lz = 0; lz < 16; ++lz) {
     for(int ly = 0; ly < 8; ++ly) {
      const bool edge =
          !filled[static_cast<std::size_t>((lx * 16 + lz) * 8 + ly)] &&
          ((lx < 15 && filled[static_cast<std::size_t>(((lx + 1) * 16 + lz) * 8 + ly)]) ||
           (lx > 0 && filled[static_cast<std::size_t>(((lx - 1) * 16 + lz) * 8 + ly)]) ||
           (lz < 15 && filled[static_cast<std::size_t>((lx * 16 + (lz + 1)) * 8 + ly)]) ||
           (lz > 0 && filled[static_cast<std::size_t>((lx * 16 + (lz - 1)) * 8 + ly)]) ||
           (ly < 7 && filled[static_cast<std::size_t>((lx * 16 + lz) * 8 + (ly + 1))]) ||
           (ly > 0 && filled[static_cast<std::size_t>((lx * 16 + lz) * 8 + (ly - 1))]));
      if(!edge || (ly >= 4 && random.nextInt(2) == 0) ||
         !world->getMaterial(x + lx, y + ly, z + lz).isSolid()) {
       continue;
      }
      world->setBlockWithoutNotifyingNeighbors(x + lx, y + ly, z + lz, Block::STONE->id);
     }
    }
   }
  }
  return true;
 }

 private:
 int waterBlockId_ = 0;
};
} // namespace net::minecraft
