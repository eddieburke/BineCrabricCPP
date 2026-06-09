#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"

namespace net::minecraft {

// Faithful 1:1 port of net.minecraft.world.gen.feature.SpringFeature.
class SpringFeature : public Feature {
public:
    explicit SpringFeature(int liquidBlockId) : liquidBlockId_(liquidBlockId) {}

    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        if (world->getBlockId(x, y + 1, z) != Block::STONE->id) {
            return false;
        }
        if (world->getBlockId(x, y - 1, z) != Block::STONE->id) {
            return false;
        }
        if (world->getBlockId(x, y, z) != 0 && world->getBlockId(x, y, z) != Block::STONE->id) {
            return false;
        }
        int n = 0;
        if (world->getBlockId(x - 1, y, z) == Block::STONE->id) {
            ++n;
        }
        if (world->getBlockId(x + 1, y, z) == Block::STONE->id) {
            ++n;
        }
        if (world->getBlockId(x, y, z - 1) == Block::STONE->id) {
            ++n;
        }
        if (world->getBlockId(x, y, z + 1) == Block::STONE->id) {
            ++n;
        }
        int n2 = 0;
        if (world->isAir(x - 1, y, z)) {
            ++n2;
        }
        if (world->isAir(x + 1, y, z)) {
            ++n2;
        }
        if (world->isAir(x, y, z - 1)) {
            ++n2;
        }
        if (world->isAir(x, y, z + 1)) {
            ++n2;
        }
        if (n == 3 && n2 == 1) {
            world->setBlock(x, y, z, liquidBlockId_);
            world->instantBlockUpdateEnabled = true;
            Block::BLOCKS[static_cast<std::size_t>(liquidBlockId_)]->onTick(world, x, y, z, random);
            world->instantBlockUpdateEnabled = false;
        }
        return true;
    }

private:
    int liquidBlockId_ = 0;
};

} // namespace net::minecraft
