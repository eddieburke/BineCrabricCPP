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
        const float f = random.nextFloat() * PI_F;
        const double d = static_cast<double>(static_cast<float>(x + 8) + MathHelper::sin(f) * oc / 8.0f);
        const double d2 = static_cast<double>(static_cast<float>(x + 8) - MathHelper::sin(f) * oc / 8.0f);
        const double d3 = static_cast<double>(static_cast<float>(z + 8) + MathHelper::cos(f) * oc / 8.0f);
        const double d4 = static_cast<double>(static_cast<float>(z + 8) - MathHelper::cos(f) * oc / 8.0f);
        const double d5 = static_cast<double>(y + random.nextInt(3) + 2);
        const double d6 = static_cast<double>(y + random.nextInt(3) + 2);
        for (int i = 0; i <= oreCount_; ++i) {
            const double d7 = d + (d2 - d) * static_cast<double>(i) / static_cast<double>(oreCount_);
            const double d8 = d5 + (d6 - d5) * static_cast<double>(i) / static_cast<double>(oreCount_);
            const double d9 = d3 + (d4 - d3) * static_cast<double>(i) / static_cast<double>(oreCount_);
            const double d10 = random.nextDouble() * static_cast<double>(oreCount_) / 16.0;
            const double d11 = static_cast<double>(MathHelper::sin(static_cast<float>(i) * PI_F / oc) + 1.0f) * d10 + 1.0;
            const double d12 = static_cast<double>(MathHelper::sin(static_cast<float>(i) * PI_F / oc) + 1.0f) * d10 + 1.0;
            const int n = MathHelper::floor(d7 - d11 / 2.0);
            const int n2 = MathHelper::floor(d8 - d12 / 2.0);
            const int n3 = MathHelper::floor(d9 - d11 / 2.0);
            const int n4 = MathHelper::floor(d7 + d11 / 2.0);
            const int n5 = MathHelper::floor(d8 + d12 / 2.0);
            const int n6 = MathHelper::floor(d9 + d11 / 2.0);
            for (int j = n; j <= n4; ++j) {
                const double d13 = (static_cast<double>(j) + 0.5 - d7) / (d11 / 2.0);
                if (!(d13 * d13 < 1.0)) {
                    continue;
                }
                for (int k = n2; k <= n5; ++k) {
                    const double d14 = (static_cast<double>(k) + 0.5 - d8) / (d12 / 2.0);
                    if (!(d13 * d13 + d14 * d14 < 1.0)) {
                        continue;
                    }
                    for (int i2 = n3; i2 <= n6; ++i2) {
                        const double d15 = (static_cast<double>(i2) + 0.5 - d9) / (d11 / 2.0);
                        if (!(d13 * d13 + d14 * d14 + d15 * d15 < 1.0) || world->getBlockId(j, k, i2) != Block::STONE->id) {
                            continue;
                        }
                        world->setBlockWithoutNotifyingNeighbors(j, k, i2, oreBlockId_);
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
