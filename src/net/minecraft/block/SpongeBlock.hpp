#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {
// Registered in Block.cpp.
class SpongeBlock : public Block {
   public:
    explicit SpongeBlock(int id) : Block(id, 48, material::Material::SPONGE) {
    }

    void onBreak(World* world, int x, int y, int z) override {
        if (world == nullptr) {
            return;
        }
        constexpr int radius = 2;
        for (int ix = x - radius; ix <= x + radius; ++ix) {
            for (int iy = y - radius; iy <= y + radius; ++iy) {
                for (int iz = z - radius; iz <= z + radius; ++iz) {
                    world->notifyNeighbors(ix, iy, iz, world->getBlockId(ix, iy, iz));
                }
            }
        }
    }
};
}  // namespace net::minecraft::block