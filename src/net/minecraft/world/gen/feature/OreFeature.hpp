#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"

namespace net::minecraft {

// Faithful 1:1 port of net.minecraft.world.gen.feature.OreFeature.
class OreFeature : public Feature {
public:
    OreFeature(int oreBlockId, int oreCount) : oreBlockId_(oreBlockId), oreCount_(oreCount) {}

    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        static constexpr float PI_F = 3.1415927f;
        const float oc = static_cast<float>(oreCount_);
        const float angle = random.nextFloat() * PI_F;
        const double startX = static_cast<double>(static_cast<float>(x + 8) + MathHelper::sin(angle) * oc / 8.0f);
        const double endX = static_cast<double>(static_cast<float>(x + 8) - MathHelper::sin(angle) * oc / 8.0f);
        const double startZ = static_cast<double>(static_cast<float>(z + 8) + MathHelper::cos(angle) * oc / 8.0f);
        const double endZ = static_cast<double>(static_cast<float>(z + 8) - MathHelper::cos(angle) * oc / 8.0f);
        const double startY = static_cast<double>(y + random.nextInt(3) + 2);
        const double endY = static_cast<double>(y + random.nextInt(3) + 2);
        for (int i = 0; i <= oreCount_; ++i) {
            const double centerX = startX + (endX - startX) * static_cast<double>(i) / static_cast<double>(oreCount_);
            const double centerY = startY + (endY - startY) * static_cast<double>(i) / static_cast<double>(oreCount_);
            const double centerZ = startZ + (endZ - startZ) * static_cast<double>(i) / static_cast<double>(oreCount_);
            const double sizeNoise = random.nextDouble() * static_cast<double>(oreCount_) / 16.0;
            const double radiusXZ = static_cast<double>(MathHelper::sin(static_cast<float>(i) * PI_F / oc) + 1.0f) * sizeNoise + 1.0;
            const double radiusY = static_cast<double>(MathHelper::sin(static_cast<float>(i) * PI_F / oc) + 1.0f) * sizeNoise + 1.0;
            const int minX = MathHelper::floor(centerX - radiusXZ / 2.0);
            const int minY = MathHelper::floor(centerY - radiusY / 2.0);
            const int minZ = MathHelper::floor(centerZ - radiusXZ / 2.0);
            const int maxX = MathHelper::floor(centerX + radiusXZ / 2.0);
            const int maxY = MathHelper::floor(centerY + radiusY / 2.0);
            const int maxZ = MathHelper::floor(centerZ + radiusXZ / 2.0);
            for (int blockX = minX; blockX <= maxX; ++blockX) {
                const double normX = (static_cast<double>(blockX) + 0.5 - centerX) / (radiusXZ / 2.0);
                if (!(normX * normX < 1.0)) {
                    continue;
                }
                for (int blockY = minY; blockY <= maxY; ++blockY) {
                    const double normY = (static_cast<double>(blockY) + 0.5 - centerY) / (radiusY / 2.0);
                    if (!(normX * normX + normY * normY < 1.0)) {
                        continue;
                    }
                    for (int blockZ = minZ; blockZ <= maxZ; ++blockZ) {
                        const double normZ = (static_cast<double>(blockZ) + 0.5 - centerZ) / (radiusXZ / 2.0);
                        if (!(normX * normX + normY * normY + normZ * normZ < 1.0) || world->getBlockId(blockX, blockY, blockZ) != Block::STONE->id) {
                            continue;
                        }
                        world->setBlockWithoutNotifyingNeighbors(blockX, blockY, blockZ, oreBlockId_);
                    }
                }
            }
        }
        return true;
    }

private:
    int oreBlockId_ = 0;
    int oreCount_ = 0;
};

} // namespace net::minecraft
