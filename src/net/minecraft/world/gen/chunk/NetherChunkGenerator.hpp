#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/util/math/noise/OctavePerlinNoiseSampler.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/gen/carver/NetherCaveCarver.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
#include "net/minecraft/world/gen/feature/GlowstoneClusterFeature.hpp"
#include "net/minecraft/world/gen/feature/GlowstoneClusterRareFeature.hpp"
#include "net/minecraft/world/gen/feature/NetherFirePatchFeature.hpp"
#include "net/minecraft/world/gen/feature/NetherLavaSpringFeature.hpp"
#include "net/minecraft/world/gen/feature/PlantPatchFeature.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace net::minecraft {

class World;

class NetherChunkGenerator : public ChunkSource {
public:
    explicit NetherChunkGenerator(World* world, std::uint64_t seed)
        : random_(seed),
          minLimitPerlinNoise_(random_, 16),
          maxLimitPerlinNoise_(random_, 16),
          perlinNoise1_(random_, 8),
          perlinNoise2_(random_, 4),
          perlinNoise3_(random_, 4),
          scaleNoise_(random_, 10),
          depthNoise_(random_, 16),
          world_(world)
    {
    }

    Chunk& loadChunk(int chunkX, int chunkZ) override
    {
        return getChunk(chunkX, chunkZ);
    }

    Chunk& getChunk(int chunkX, int chunkZ) override
    {
        random_.setSeed(static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkX) * 341873128712LL + static_cast<std::int64_t>(chunkZ) * 132897987541LL));
        Chunk chunk(world_, chunkX, chunkZ);
        buildTerrain(chunkX, chunkZ, chunk);
        buildSurfaces(chunkX, chunkZ, chunk);
        cave_.place(this, world_, chunkX, chunkZ, chunk);
        if (world_ != nullptr) {
            chunk.populateHeightMap();
        }
        scratchChunk_.emplace(std::move(chunk));
        return *scratchChunk_;
    }

    [[nodiscard]] bool isChunkLoaded(int /*chunkX*/, int /*chunkZ*/) const override
    {
        return true;
    }

    void decorate(ChunkSource* /*source*/, int chunkX, int chunkZ) override
    {
        block::FallingBlock::fallInstantly = true;
        const int blockOriginX = chunkX * 16;
        const int blockOriginZ = chunkZ * 16;
        for (int i = 0; i < 8; ++i) {
            NetherLavaSpringFeature(Block::FLOWING_LAVA->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8,
                random_.nextInt(120) + 4, blockOriginZ + random_.nextInt(16) + 8);
        }
        const int firePatches = random_.nextInt(random_.nextInt(10) + 1) + 1;
        for (int i = 0; i < firePatches; ++i) {
            NetherFirePatchFeature().generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(120) + 4,
                blockOriginZ + random_.nextInt(16) + 8);
        }
        const int glowstoneClusters = random_.nextInt(random_.nextInt(10) + 1);
        for (int i = 0; i < glowstoneClusters; ++i) {
            GlowstoneClusterFeature().generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(120) + 4,
                blockOriginZ + random_.nextInt(16) + 8);
        }
        for (int i = 0; i < 10; ++i) {
            GlowstoneClusterRareFeature().generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        if (random_.nextInt(1) == 0) {
            PlantPatchFeature(Block::BROWN_MUSHROOM->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8,
                random_.nextInt(128), blockOriginZ + random_.nextInt(16) + 8);
        }
        if (random_.nextInt(1) == 0) {
            PlantPatchFeature(Block::RED_MUSHROOM->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8,
                random_.nextInt(128), blockOriginZ + random_.nextInt(16) + 8);
        }
        block::FallingBlock::fallInstantly = false;
    }

    bool save(bool /*saveEntities*/, client::gui::screen::LoadingDisplay* /*display*/) override
    {
        return true;
    }

    bool tick() override { return false; }
    [[nodiscard]] bool canSave() const override { return true; }
    [[nodiscard]] std::string getDebugInfo() const override { return "HellRandomLevelSource"; }

private:
    void buildTerrain(int chunkX, int chunkZ, Chunk& chunk)
    {
        static constexpr int cellSize = 4;
        static constexpr int lavaLevel = 32;
        static constexpr int sizeX = cellSize + 1;
        static constexpr int sizeY = 17;
        static constexpr int sizeZ = cellSize + 1;
        heightMap_ = generateHeightMap(heightMap_, chunkX * cellSize, 0, chunkZ * cellSize, sizeX, sizeY, sizeZ);

        for (int cellX = 0; cellX < cellSize; ++cellX) {
            for (int cellZ = 0; cellZ < cellSize; ++cellZ) {
                for (int cellY = 0; cellY < 16; ++cellY) {
                    double density00 = heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
                    double density01 = heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
                    double density10 = heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
                    double density11 = heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
                    const double dy0 = (heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] - density00) * 0.125;
                    const double dy1 = (heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] - density01) * 0.125;
                    const double dy2 = (heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] - density10) * 0.125;
                    const double dy3 = (heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] - density11) * 0.125;

                    for (int subY = 0; subY < 8; ++subY) {
                        double x0 = density00;
                        double x1 = density01;
                        const double dx0 = (density10 - density00) * 0.25;
                        const double dx1 = (density11 - density01) * 0.25;
                        for (int subX = 0; subX < 4; ++subX) {
                            double density = x0;
                            const double dz = (x1 - x0) * 0.25;
                            for (int subZ = 0; subZ < 4; ++subZ) {
                                const int localX = cellX * 4 + subX;
                                const int localY = cellY * 8 + subY;
                                const int localZ = cellZ * 4 + subZ;
                                int blockId = 0;
                                if (localY < lavaLevel) {
                                    blockId = Block::LAVA->id;
                                }
                                if (density > 0.0) {
                                    blockId = Block::NETHERRACK->id;
                                }
                                Generator::setRawBlock(chunk, localX, localY, localZ, blockId);
                                density += dz;
                            }
                            x0 += dx0;
                            x1 += dx1;
                        }
                        density00 += dy0;
                        density01 += dy1;
                        density10 += dy2;
                        density11 += dy3;
                    }
                }
            }
        }
    }

    void buildSurfaces(int chunkX, int chunkZ, Chunk& chunk)
    {
        static constexpr int seaLevel = 64;
        constexpr double scale = 0.03125;
        sandBuffer_ = perlinNoise2_.create(sandBuffer_, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, scale, scale, 1.0);
        gravelBuffer_ = perlinNoise2_.create(gravelBuffer_, chunkX * 16, 109.0134, chunkZ * 16, 16, 1, 16, scale, 1.0, scale);
        depthBuffer_ = perlinNoise3_.create(depthBuffer_, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, scale * 2.0, scale * 2.0, scale * 2.0);

        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                const bool sand = sandBuffer_[static_cast<std::size_t>(x + z * 16)] + random_.nextDouble() * 0.2 > 0.0;
                const bool gravel = gravelBuffer_[static_cast<std::size_t>(x + z * 16)] + random_.nextDouble() * 0.2 > 0.0;
                int depth = static_cast<int>(depthBuffer_[static_cast<std::size_t>(x + z * 16)] / 3.0 + 3.0 + random_.nextDouble() * 0.25);
                int run = -1;
                std::uint8_t top = static_cast<std::uint8_t>(Block::NETHERRACK->id);
                std::uint8_t soil = static_cast<std::uint8_t>(Block::NETHERRACK->id);

                for (int y = 127; y >= 0; --y) {
                    if (y >= 127 - random_.nextInt(5)) {
                        Generator::setRawBlock(chunk, x, y, z, Block::BEDROCK->id);
                        continue;
                    }
                    if (y <= random_.nextInt(5)) {
                        Generator::setRawBlock(chunk, x, y, z, Block::BEDROCK->id);
                        continue;
                    }
                    const int blockId = Generator::rawBlock(chunk, x, y, z);
                    if (blockId == 0) {
                        run = -1;
                        continue;
                    }
                    if (blockId != Block::NETHERRACK->id) {
                        continue;
                    }
                    if (run == -1) {
                        if (depth <= 0) {
                            top = 0;
                            soil = static_cast<std::uint8_t>(Block::NETHERRACK->id);
                        } else if (y >= seaLevel - 4 && y <= seaLevel + 1) {
                            top = static_cast<std::uint8_t>(Block::NETHERRACK->id);
                            soil = static_cast<std::uint8_t>(Block::NETHERRACK->id);
                            if (gravel) {
                                top = static_cast<std::uint8_t>(Block::GRAVEL->id);
                            }
                            if (sand) {
                                top = static_cast<std::uint8_t>(Block::SOUL_SAND->id);
                                soil = static_cast<std::uint8_t>(Block::SOUL_SAND->id);
                            }
                        }
                        if (y < seaLevel && top == 0) {
                            top = static_cast<std::uint8_t>(Block::LAVA->id);
                        }
                        run = depth;
                        Generator::setRawBlock(chunk, x, y, z, y >= seaLevel - 1 ? top : soil);
                        continue;
                    }
                    if (run <= 0) {
                        continue;
                    }
                    --run;
                    Generator::setRawBlock(chunk, x, y, z, soil);
                }
            }
        }
    }

    std::vector<double>& generateHeightMap(std::vector<double>& heightMap, int x, int y, int z, int sizeX, int sizeY, int sizeZ)
    {
        const std::size_t size = static_cast<std::size_t>(sizeX * sizeY * sizeZ);
        if (heightMap.size() < size) {
            heightMap.assign(size, 0.0);
        }
        constexpr double baseScale = 684.412;
        constexpr double verticalScale = 2053.236;
        scaleNoiseBuffer_ = scaleNoise_.create(scaleNoiseBuffer_, x, y, z, sizeX, 1, sizeZ, 1.0, 0.0, 1.0);
        depthNoiseBuffer_ = depthNoise_.create(depthNoiseBuffer_, x, y, z, sizeX, 1, sizeZ, 100.0, 0.0, 100.0);
        perlinNoiseBuffer_ = perlinNoise1_.create(perlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale / 80.0, verticalScale / 60.0, baseScale / 80.0);
        minLimitPerlinNoiseBuffer_ = minLimitPerlinNoise_.create(minLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, verticalScale, baseScale);
        maxLimitPerlinNoiseBuffer_ = maxLimitPerlinNoise_.create(maxLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, verticalScale, baseScale);

        std::vector<double> verticalCurve(static_cast<std::size_t>(sizeY));
        for (int iy = 0; iy < sizeY; ++iy) {
            verticalCurve[static_cast<std::size_t>(iy)] = std::cos(static_cast<double>(iy) * 3.141592653589793 * 6.0 / static_cast<double>(sizeY)) * 2.0;
            double distanceFromEdge = static_cast<double>(iy);
            if (iy > sizeY / 2) {
                distanceFromEdge = static_cast<double>(sizeY - 1 - iy);
            }
            if (distanceFromEdge < 4.0) {
                distanceFromEdge = 4.0 - distanceFromEdge;
                verticalCurve[static_cast<std::size_t>(iy)] -= distanceFromEdge * distanceFromEdge * distanceFromEdge * 10.0;
            }
        }

        int index = 0;
        int horizontalIndex = 0;
        for (int ix = 0; ix < sizeX; ++ix) {
            for (int iz = 0; iz < sizeZ; ++iz) {
                double scaleNoise = (scaleNoiseBuffer_[static_cast<std::size_t>(horizontalIndex)] + 256.0) / 512.0;
                if (scaleNoise > 1.0) {
                    scaleNoise = 1.0;
                }
                double depthNoise = depthNoiseBuffer_[static_cast<std::size_t>(horizontalIndex)] / 8000.0;
                if (depthNoise < 0.0) {
                    depthNoise = -depthNoise;
                }
                depthNoise = depthNoise * 3.0 - 3.0;
                if (depthNoise < 0.0) {
                    depthNoise = std::max(depthNoise / 2.0, -1.0);
                    depthNoise /= 1.4;
                    depthNoise /= 2.0;
                    scaleNoise = 0.0;
                } else {
                    depthNoise = std::min(depthNoise, 1.0) / 6.0;
                }
                scaleNoise += 0.5;
                depthNoise = depthNoise * static_cast<double>(sizeY) / 16.0;
                ++horizontalIndex;

                for (int iy = 0; iy < sizeY; ++iy) {
                    double vertical = verticalCurve[static_cast<std::size_t>(iy)];
                    const double minLimit = minLimitPerlinNoiseBuffer_[static_cast<std::size_t>(index)] / 512.0;
                    const double maxLimit = maxLimitPerlinNoiseBuffer_[static_cast<std::size_t>(index)] / 512.0;
                    const double selector = (perlinNoiseBuffer_[static_cast<std::size_t>(index)] / 10.0 + 1.0) / 2.0;
                    double density = selector < 0.0 ? minLimit : (selector > 1.0 ? maxLimit : minLimit + (maxLimit - minLimit) * selector);
                    density -= vertical;
                    if (iy > sizeY - 4) {
                        const double topFade = static_cast<double>(iy - (sizeY - 4)) / 3.0;
                        density = density * (1.0 - topFade) + -10.0 * topFade;
                    }
                    heightMap[static_cast<std::size_t>(index++)] = density;
                }
            }
        }
        return heightMap;
    }

    JavaRandom random_;
    OctavePerlinNoiseSampler minLimitPerlinNoise_;
    OctavePerlinNoiseSampler maxLimitPerlinNoise_;
    OctavePerlinNoiseSampler perlinNoise1_;
    OctavePerlinNoiseSampler perlinNoise2_;
    OctavePerlinNoiseSampler perlinNoise3_;
    OctavePerlinNoiseSampler scaleNoise_;
    OctavePerlinNoiseSampler depthNoise_;
    World* world_ = nullptr;
    std::vector<double> heightMap_;
    std::vector<double> sandBuffer_;
    std::vector<double> gravelBuffer_;
    std::vector<double> depthBuffer_;
    NetherCaveCarver cave_;
    std::vector<double> perlinNoiseBuffer_;
    std::vector<double> minLimitPerlinNoiseBuffer_;
    std::vector<double> maxLimitPerlinNoiseBuffer_;
    std::vector<double> scaleNoiseBuffer_;
    std::vector<double> depthNoiseBuffer_;
    std::optional<Chunk> scratchChunk_;
};

} // namespace net::minecraft
