#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/LightType.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"

#include <array>

namespace net::minecraft {

// Faithful 1:1 port of net.minecraft.world.gen.feature.LakeFeature.
class LakeFeature : public Feature {
public:
    explicit LakeFeature(int waterBlockId) : waterBlockId_(waterBlockId) {}

    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        x -= 8;
        z -= 8;
        while (y > 0 && world->isAir(x, y, z)) {
            --y;
        }
        y -= 4;
        std::array<bool, 2048> filled {};
        const int blobs = random.nextInt(4) + 4;
        for (int b = 0; b < blobs; ++b) {
            const double d = random.nextDouble() * 6.0 + 3.0;
            const double d2 = random.nextDouble() * 4.0 + 2.0;
            const double d3 = random.nextDouble() * 6.0 + 3.0;
            const double d4 = random.nextDouble() * (16.0 - d - 2.0) + 1.0 + d / 2.0;
            const double d5 = random.nextDouble() * (8.0 - d2 - 4.0) + 2.0 + d2 / 2.0;
            const double d6 = random.nextDouble() * (16.0 - d3 - 2.0) + 1.0 + d3 / 2.0;
            for (int i = 1; i < 15; ++i) {
                for (int j = 1; j < 15; ++j) {
                    for (int k = 1; k < 7; ++k) {
                        const double d7 = (static_cast<double>(i) - d4) / (d / 2.0);
                        const double d8 = (static_cast<double>(k) - d5) / (d2 / 2.0);
                        const double d9 = (static_cast<double>(j) - d6) / (d3 / 2.0);
                        if (!(d7 * d7 + d8 * d8 + d9 * d9 < 1.0)) {
                            continue;
                        }
                        filled[static_cast<std::size_t>((i * 16 + j) * 8 + k)] = true;
                    }
                }
            }
        }

        for (int n2 = 0; n2 < 16; ++n2) {
            for (int i = 0; i < 16; ++i) {
                for (int n = 0; n < 8; ++n) {
                    const bool edge = !filled[static_cast<std::size_t>((n2 * 16 + i) * 8 + n)]
                        && ((n2 < 15 && filled[static_cast<std::size_t>(((n2 + 1) * 16 + i) * 8 + n)])
                            || (n2 > 0 && filled[static_cast<std::size_t>(((n2 - 1) * 16 + i) * 8 + n)])
                            || (i < 15 && filled[static_cast<std::size_t>((n2 * 16 + (i + 1)) * 8 + n)])
                            || (i > 0 && filled[static_cast<std::size_t>((n2 * 16 + (i - 1)) * 8 + n)])
                            || (n < 7 && filled[static_cast<std::size_t>((n2 * 16 + i) * 8 + (n + 1))])
                            || (n > 0 && filled[static_cast<std::size_t>((n2 * 16 + i) * 8 + (n - 1))]));
                    if (!edge) {
                        continue;
                    }
                    block::material::Material& material = world->getMaterial(x + n2, y + n, z + i);
                    if (n >= 4 && material.isFluid()) {
                        return false;
                    }
                    if (n >= 4 || material.isSolid() || world->getBlockId(x + n2, y + n, z + i) == waterBlockId_) {
                        continue;
                    }
                    return false;
                }
            }
        }

        for (int n2 = 0; n2 < 16; ++n2) {
            for (int i = 0; i < 16; ++i) {
                for (int n = 0; n < 8; ++n) {
                    if (!filled[static_cast<std::size_t>((n2 * 16 + i) * 8 + n)]) {
                        continue;
                    }
                    world->setBlockWithoutNotifyingNeighbors(x + n2, y + n, z + i, n >= 4 ? 0 : waterBlockId_);
                }
            }
        }

        for (int n2 = 0; n2 < 16; ++n2) {
            for (int i = 0; i < 16; ++i) {
                for (int n = 4; n < 8; ++n) {
                    if (!filled[static_cast<std::size_t>((n2 * 16 + i) * 8 + n)]
                        || world->getBlockId(x + n2, y + n - 1, z + i) != Block::DIRT->id
                        || world->getBrightness(LightType::Sky, x + n2, y + n, z + i) <= 0) {
                        continue;
                    }
                    world->setBlockWithoutNotifyingNeighbors(x + n2, y + n - 1, z + i, Block::GRASS_BLOCK->id);
                }
            }
        }

        if (&Block::BLOCKS[static_cast<std::size_t>(waterBlockId_)]->material == &block::material::Material::LAVA) {
            for (int n2 = 0; n2 < 16; ++n2) {
                for (int i = 0; i < 16; ++i) {
                    for (int n = 0; n < 8; ++n) {
                        const bool edge = !filled[static_cast<std::size_t>((n2 * 16 + i) * 8 + n)]
                            && ((n2 < 15 && filled[static_cast<std::size_t>(((n2 + 1) * 16 + i) * 8 + n)])
                                || (n2 > 0 && filled[static_cast<std::size_t>(((n2 - 1) * 16 + i) * 8 + n)])
                                || (i < 15 && filled[static_cast<std::size_t>((n2 * 16 + (i + 1)) * 8 + n)])
                                || (i > 0 && filled[static_cast<std::size_t>((n2 * 16 + (i - 1)) * 8 + n)])
                                || (n < 7 && filled[static_cast<std::size_t>((n2 * 16 + i) * 8 + (n + 1))])
                                || (n > 0 && filled[static_cast<std::size_t>((n2 * 16 + i) * 8 + (n - 1))]));
                        if (!edge || (n >= 4 && random.nextInt(2) == 0) || !world->getMaterial(x + n2, y + n, z + i).isSolid()) {
                            continue;
                        }
                        world->setBlockWithoutNotifyingNeighbors(x + n2, y + n, z + i, Block::STONE->id);
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
