#pragma once

#include "net/minecraft/block/PlantBlock.hpp"
#include "net/minecraft/world/gen/feature/BirchTreeFeature.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/LargeOakTreeFeature.hpp"
#include "net/minecraft/world/gen/feature/OakTreeFeature.hpp"
#include "net/minecraft/world/gen/feature/SpruceTreeFeature.hpp"

#include <memory>

namespace net::minecraft::block {

class SaplingBlock : public PlantBlock {
public:
    static void registerClass();
    static void registerBlockItems();
    SaplingBlock(int id, int textureId) : PlantBlock(id, textureId)
    {
        const float f = 0.4f;
        setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f);
    }

    void onTick(World* world, int x, int y, int z, JavaRandom& random) override
    {
        if (world == nullptr || world->isRemote()) {
            return;
        }
        PlantBlock::onTick(world, x, y, z, random);
        if (world->getLightLevel(x, y + 1, z) >= 9 && random.nextInt(30) == 0) {
            const int meta = world->getBlockMeta(x, y, z);
            if ((meta & 8) == 0) {
                world->setBlockMeta(x, y, z, meta | 8);
            } else {
                generate(world, x, y, z, random);
            }
        }
    }

    [[nodiscard]] int getTexture(int side, int meta) const override
    {
        meta &= 3;
        if (meta == 1) {
            return 63;
        }
        if (meta == 2) {
            return 79;
        }
        return PlantBlock::getTexture(side, meta);
    }

    void generate(World* world, int x, int y, int z, JavaRandom& random)
    {
        if (world == nullptr) {
            return;
        }
        const int meta = world->getBlockMeta(x, y, z) & 3;
        world->setBlockWithoutNotifyingNeighbors(x, y, z, 0);

        std::unique_ptr<Feature> feature;
        if (meta == 1) {
            feature = std::make_unique<SpruceTreeFeature>();
        } else if (meta == 2) {
            feature = std::make_unique<BirchTreeFeature>();
        } else if (random.nextInt(10) == 0) {
            feature = std::make_unique<LargeOakTreeFeature>();
        } else {
            feature = std::make_unique<OakTreeFeature>();
        }

        if (!feature->generate(world, random, x, y, z)) {
            world->setBlockWithoutNotifyingNeighbors(x, y, z, id, meta);
        }
    }

protected:
    [[nodiscard]] int getDroppedItemMeta(int blockMeta) const override
    {
        return blockMeta & 3;
    }
};

} // namespace net::minecraft::block
