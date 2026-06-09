#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/util/math/noise/OctavePerlinNoiseSampler.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/BiomeTreeFeature.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/gen/carver/CaveWorldCarver.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
#include "net/minecraft/world/gen/feature/CactusPatchFeature.hpp"
#include "net/minecraft/world/gen/feature/ClayOreFeature.hpp"
#include "net/minecraft/world/gen/feature/DungeonFeature.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/LakeFeature.hpp"
#include "net/minecraft/world/gen/feature/OreFeature.hpp"
#include "net/minecraft/world/gen/feature/PlantPatchFeature.hpp"
#include "net/minecraft/world/gen/feature/PumpkinPatchFeature.hpp"
#include "net/minecraft/world/gen/feature/SpringFeature.hpp"
#include "net/minecraft/world/gen/feature/SugarCanePatchFeature.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace net::minecraft {

class SkyChunkGenerator : public ChunkSource {
public:
    explicit SkyChunkGenerator(World* world, std::uint64_t seed)
        : random_(seed),
          world_(world),
          minLimitPerlinNoise_(random_, 16),
          maxLimitPerlinNoise_(random_, 16),
          perlinNoise1_(random_, 8),
          perlinNoise2_(random_, 4),
          perlinNoise3_(random_, 4),
          floatingIslandScale_(random_, 10),
          floatingIslandNoise_(random_, 16),
          forestNoise_(random_, 8),
          biomeSource_(seed)
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
        BiomeSource* biomeSource = activeBiomeSource();
        biomes_ = biomeSource->getBiomesInArea(biomes_, chunkX * 16, chunkZ * 16, 16, 16);
        buildTerrain(chunkX, chunkZ, chunk, biomeSource->temperatureMap());
        buildSurfaces(chunkX, chunkZ, chunk, biomes_);
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
        BiomeSource* biomeSource = activeBiomeSource();
        const BiomeInfo biome = biomeSource->getBiome(blockOriginX + 16, blockOriginZ + 16);
        random_.setSeed(world_->getSeed());
        const std::int64_t l = random_.nextLong() / 2LL * 2LL + 1LL;
        const std::int64_t l2 = random_.nextLong() / 2LL * 2LL + 1LL;
        random_.setSeed(static_cast<std::uint64_t>(
            (static_cast<std::int64_t>(chunkX) * l + static_cast<std::int64_t>(chunkZ) * l2)
            ^ static_cast<std::int64_t>(world_->getSeed())));

        if (random_.nextInt(4) == 0) {
            LakeFeature(Block::WATER->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        if (random_.nextInt(8) == 0) {
            const int lakeY = random_.nextInt(random_.nextInt(120) + 8);
            if (lakeY < 64 || random_.nextInt(10) == 0) {
                LakeFeature(Block::LAVA->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, lakeY,
                    blockOriginZ + random_.nextInt(16) + 8);
            }
        }
        for (int i = 0; i < 8; ++i) {
            DungeonFeature().generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        for (int i = 0; i < 10; ++i) {
            ClayOreFeature(32).generate(world_, random_, blockOriginX + random_.nextInt(16), random_.nextInt(128),
                blockOriginZ + random_.nextInt(16));
        }
        for (int i = 0; i < 20; ++i) {
            OreFeature(Block::DIRT->id, 32).generate(world_, random_, blockOriginX + random_.nextInt(16), random_.nextInt(128),
                blockOriginZ + random_.nextInt(16));
        }
        for (int i = 0; i < 10; ++i) {
            OreFeature(Block::GRAVEL->id, 32).generate(world_, random_, blockOriginX + random_.nextInt(16), random_.nextInt(128),
                blockOriginZ + random_.nextInt(16));
        }
        for (int i = 0; i < 20; ++i) {
            OreFeature(Block::COAL_ORE->id, 16).generate(world_, random_, blockOriginX + random_.nextInt(16), random_.nextInt(128),
                blockOriginZ + random_.nextInt(16));
        }
        for (int i = 0; i < 20; ++i) {
            OreFeature(Block::IRON_ORE->id, 8).generate(world_, random_, blockOriginX + random_.nextInt(16), random_.nextInt(64),
                blockOriginZ + random_.nextInt(16));
        }
        for (int i = 0; i < 2; ++i) {
            OreFeature(Block::GOLD_ORE->id, 8).generate(world_, random_, blockOriginX + random_.nextInt(16), random_.nextInt(32),
                blockOriginZ + random_.nextInt(16));
        }
        for (int i = 0; i < 8; ++i) {
            OreFeature(Block::REDSTONE_ORE->id, 7).generate(world_, random_, blockOriginX + random_.nextInt(16), random_.nextInt(16),
                blockOriginZ + random_.nextInt(16));
        }
        for (int i = 0; i < 1; ++i) {
            OreFeature(Block::DIAMOND_ORE->id, 7).generate(world_, random_, blockOriginX + random_.nextInt(16), random_.nextInt(16),
                blockOriginZ + random_.nextInt(16));
        }
        for (int i = 0; i < 1; ++i) {
            OreFeature(Block::LAPIS_ORE->id, 6).generate(world_, random_, blockOriginX + random_.nextInt(16),
                random_.nextInt(16) + random_.nextInt(16), blockOriginZ + random_.nextInt(16));
        }

        constexpr double forestScale = 0.5;
        const int forestRoll = static_cast<int>((forestNoise_.sample(static_cast<double>(blockOriginX) * forestScale, static_cast<double>(blockOriginZ) * forestScale) / 8.0
                                                   + random_.nextDouble() * 4.0 + 4.0)
            / 3.0);
        int trees = 0;
        if (random_.nextInt(10) == 0) {
            ++trees;
        }
        if (biome.id == BiomeId::Forest) {
            trees += forestRoll + 5;
        } else if (biome.id == BiomeId::Rainforest) {
            trees += forestRoll + 5;
        } else if (biome.id == BiomeId::SeasonalForest) {
            trees += forestRoll + 2;
        } else if (biome.id == BiomeId::Taiga) {
            trees += forestRoll + 5;
        } else if (biome.id == BiomeId::Desert || biome.id == BiomeId::Tundra || biome.id == BiomeId::Plains) {
            trees -= 20;
        }
        for (int i = 0; i < trees; ++i) {
            const int treeX = blockOriginX + random_.nextInt(16) + 8;
            const int treeZ = blockOriginZ + random_.nextInt(16) + 8;
            std::unique_ptr<Feature> feature = getRandomTreeFeature(biome.id, random_);
            feature->prepare(1.0, 1.0, 1.0);
            feature->generate(world_, random_, treeX, world_->getTopY(treeX, treeZ), treeZ);
        }
        for (int i = 0; i < 2; ++i) {
            PlantPatchFeature(Block::DANDELION->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        if (random_.nextInt(2) == 0) {
            PlantPatchFeature(Block::ROSE->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        if (random_.nextInt(4) == 0) {
            PlantPatchFeature(Block::BROWN_MUSHROOM->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        if (random_.nextInt(8) == 0) {
            PlantPatchFeature(Block::RED_MUSHROOM->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        for (int i = 0; i < 10; ++i) {
            SugarCanePatchFeature().generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        if (random_.nextInt(32) == 0) {
            PumpkinPatchFeature().generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        int cactusPatches = biome.id == BiomeId::Desert ? 10 : 0;
        for (int i = 0; i < cactusPatches; ++i) {
            CactusPatchFeature().generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
        for (int i = 0; i < 50; ++i) {
            SpringFeature(Block::FLOWING_WATER->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8,
                random_.nextInt(random_.nextInt(120) + 8), blockOriginZ + random_.nextInt(16) + 8);
        }
        for (int i = 0; i < 20; ++i) {
            SpringFeature(Block::FLOWING_LAVA->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8,
                random_.nextInt(random_.nextInt(random_.nextInt(112) + 8) + 8), blockOriginZ + random_.nextInt(16) + 8);
        }

        decorateTemperatures_ = biomeSource->create(decorateTemperatures_, blockOriginX + 8, blockOriginZ + 8, 16, 16);
        for (int wx = blockOriginX + 8; wx < blockOriginX + 8 + 16; ++wx) {
            for (int wz = blockOriginZ + 8; wz < blockOriginZ + 8 + 16; ++wz) {
                const int localX = wx - (blockOriginX + 8);
                const int localZ = wz - (blockOriginZ + 8);
                const int topSolidY = world_->getTopSolidBlockY(wx, wz);
                const double temperature = decorateTemperatures_[static_cast<std::size_t>(localX * 16 + localZ)]
                    - static_cast<double>(topSolidY - 64) / 64.0 * 0.3;
                if (temperature >= 0.5 || topSolidY <= 0 || topSolidY >= 128 || !world_->isAir(wx, topSolidY, wz)) {
                    continue;
                }
                block::material::Material& groundMaterial = world_->getMaterial(wx, topSolidY - 1, wz);
                if (!groundMaterial.blocksMovement() || &groundMaterial == &block::material::Material::ICE) {
                    continue;
                }
                world_->setBlock(wx, topSolidY, wz, Block::SNOW->id);
            }
        }
        block::FallingBlock::fallInstantly = false;
    }

    bool save(bool /*saveEntities*/, client::gui::screen::LoadingDisplay* /*display*/) override
    {
        return true;
    }

    bool tick() override { return false; }
    [[nodiscard]] bool canSave() const override { return true; }
    [[nodiscard]] std::string getDebugInfo() const override { return "RandomLevelSource"; }

private:
    [[nodiscard]] BiomeSource* activeBiomeSource()
    {
        if (world_ != nullptr && world_->getBiomeSource() != nullptr) {
            return world_->getBiomeSource();
        }
        return &biomeSource_;
    }

    [[nodiscard]] const BiomeSource* activeBiomeSource() const
    {
        if (world_ != nullptr && world_->getBiomeSource() != nullptr) {
            return world_->getBiomeSource();
        }
        return &biomeSource_;
    }

    void buildTerrain(int chunkX, int chunkZ, Chunk& chunk, const std::vector<double>& /*temperatures*/)
    {
        static constexpr int cellSize = 2;
        static constexpr int sizeX = cellSize + 1;
        static constexpr int sizeY = 33;
        static constexpr int sizeZ = cellSize + 1;
        heightMap_ = generateHeightMap(heightMap_, chunkX * cellSize, 0, chunkZ * cellSize, sizeX, sizeY, sizeZ);

        for (int cellX = 0; cellX < cellSize; ++cellX) {
            for (int cellZ = 0; cellZ < cellSize; ++cellZ) {
                for (int cellY = 0; cellY < 32; ++cellY) {
                    double d0 = heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
                    double d1 = heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
                    double d2 = heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
                    double d3 = heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
                    const double dy0 = (heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] - d0) * 0.25;
                    const double dy1 = (heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] - d1) * 0.25;
                    const double dy2 = (heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] - d2) * 0.25;
                    const double dy3 = (heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] - d3) * 0.25;

                    for (int subY = 0; subY < 4; ++subY) {
                        double x0 = d0;
                        double x1 = d1;
                        const double dx0 = (d2 - d0) * 0.125;
                        const double dx1 = (d3 - d1) * 0.125;
                        for (int subX = 0; subX < 8; ++subX) {
                            double density = x0;
                            const double dz = (x1 - x0) * 0.125;
                            for (int subZ = 0; subZ < 8; ++subZ) {
                                const int localX = cellX * 8 + subX;
                                const int localY = cellY * 4 + subY;
                                const int localZ = cellZ * 8 + subZ;
                                int blockId = 0;
                                if (density > 0.0) {
                                    blockId = Block::STONE->id;
                                }
                                Generator::setRawBlock(chunk, localX, localY, localZ, blockId);
                                density += dz;
                            }
                            x0 += dx0;
                            x1 += dx1;
                        }
                        d0 += dy0;
                        d1 += dy1;
                        d2 += dy2;
                        d3 += dy3;
                    }
                }
            }
        }
    }

    void buildSurfaces(int chunkX, int chunkZ, Chunk& chunk, const std::vector<BiomeInfo>& biomes)
    {
        constexpr double scale = 0.03125;
        sandBuffer_ = perlinNoise2_.create(sandBuffer_, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, scale, scale, 1.0);
        gravelBuffer_ = perlinNoise2_.create(gravelBuffer_, chunkX * 16, 109.0134, chunkZ * 16, 16, 1, 16, scale, 1.0, scale);
        depthBuffer_ = perlinNoise3_.create(depthBuffer_, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, scale * 2.0, scale * 2.0, scale * 2.0);

        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                const BiomeInfo& biome = biomes[static_cast<std::size_t>(x + z * 16)];
                int depth = static_cast<int>(depthBuffer_[static_cast<std::size_t>(x + z * 16)] / 3.0 + 3.0 + random_.nextDouble() * 0.25);
                int run = -1;
                std::uint8_t top = biome.topBlockId;
                std::uint8_t soil = biome.soilBlockId;

                for (int y = 127; y >= 0; --y) {
                    const int blockId = Generator::rawBlock(chunk, x, y, z);
                    if (blockId == 0) {
                        run = -1;
                        continue;
                    }
                    if (blockId != Block::STONE->id) {
                        continue;
                    }
                    if (run == -1) {
                        if (depth <= 0) {
                            top = 0;
                            soil = static_cast<std::uint8_t>(Block::STONE->id);
                        }
                        run = depth;
                        Generator::setRawBlock(chunk, x, y, z, y >= 0 ? top : soil);
                        continue;
                    }
                    if (run <= 0) {
                        continue;
                    }
                    Generator::setRawBlock(chunk, x, y, z, soil);
                    --run;
                    if (run == 0 && soil == static_cast<std::uint8_t>(Block::SAND->id)) {
                        run = random_.nextInt(4);
                        soil = static_cast<std::uint8_t>(Block::SANDSTONE->id);
                    }
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
        double baseScale = 684.412;
        const double verticalScale = baseScale;
        baseScale *= 2.0;
        const BiomeSource* biomeSource = activeBiomeSource();
        const std::vector<double>& temperatureMap = biomeSource->temperatureMap();
        const std::vector<double>& downfallMap = biomeSource->downfallMap();
        scaleNoiseBuffer_ = floatingIslandScale_.create(scaleNoiseBuffer_, x, z, sizeX, sizeZ, 1.121, 1.121, 0.5);
        depthNoiseBuffer_ = floatingIslandNoise_.create(depthNoiseBuffer_, x, z, sizeX, sizeZ, 200.0, 200.0, 0.5);
        perlinNoiseBuffer_ = perlinNoise1_.create(perlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale / 80.0, verticalScale / 160.0, baseScale / 80.0);
        minLimitPerlinNoiseBuffer_ = minLimitPerlinNoise_.create(minLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, verticalScale, baseScale);
        maxLimitPerlinNoiseBuffer_ = maxLimitPerlinNoise_.create(maxLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, verticalScale, baseScale);

        int index = 0;
        int horizontalIndex = 0;
        const int biomeStep = 16 / sizeX;
        for (int ix = 0; ix < sizeX; ++ix) {
            const int biomeX = ix * biomeStep + biomeStep / 2;
            for (int iz = 0; iz < sizeZ; ++iz) {
                const int biomeZ = iz * biomeStep + biomeStep / 2;
                const double temperature = temperatureMap[static_cast<std::size_t>(biomeX * 16 + biomeZ)];
                const double downfallTemperature = downfallMap[static_cast<std::size_t>(biomeX * 16 + biomeZ)] * temperature;
                double terrainFactor = 1.0 - downfallTemperature;
                terrainFactor *= terrainFactor;
                terrainFactor *= terrainFactor;
                terrainFactor = 1.0 - terrainFactor;

                double scaleNoise = (scaleNoiseBuffer_[static_cast<std::size_t>(horizontalIndex)] + 256.0) / 512.0;
                scaleNoise = std::min(scaleNoise * terrainFactor, 1.0);
                double depthNoise = depthNoiseBuffer_[static_cast<std::size_t>(horizontalIndex)] / 8000.0;
                if (depthNoise < 0.0) {
                    depthNoise = -depthNoise * 0.3;
                }
                depthNoise = depthNoise * 3.0 - 2.0;
                if (depthNoise > 1.0) {
                    depthNoise = 1.0;
                }
                depthNoise /= 8.0;
                depthNoise = 0.0;
                if (scaleNoise < 0.0) {
                    scaleNoise = 0.0;
                }
                scaleNoise += 0.5;
                depthNoise = depthNoise * static_cast<double>(sizeY) / 16.0;
                ++horizontalIndex;
                const double center = static_cast<double>(sizeY) / 2.0;

                for (int iy = 0; iy < sizeY; ++iy) {
                    double vertical = (static_cast<double>(iy) - center) * 8.0 / scaleNoise;
                    if (vertical < 0.0) {
                        vertical *= -1.0;
                    }
                    const double minLimit = minLimitPerlinNoiseBuffer_[static_cast<std::size_t>(index)] / 512.0;
                    const double maxLimit = maxLimitPerlinNoiseBuffer_[static_cast<std::size_t>(index)] / 512.0;
                    const double selector = (perlinNoiseBuffer_[static_cast<std::size_t>(index)] / 10.0 + 1.0) / 2.0;
                    double density = selector < 0.0 ? minLimit : (selector > 1.0 ? maxLimit : minLimit + (maxLimit - minLimit) * selector);
                    density -= 8.0;
                    constexpr int topFadeHeight = 32;
                    if (iy > sizeY - topFadeHeight) {
                        const double topFade = static_cast<double>(iy - (sizeY - topFadeHeight)) / static_cast<double>(topFadeHeight - 1);
                        density = density * (1.0 - topFade) + -30.0 * topFade;
                    }
                    constexpr int bottomFadeHeight = 8;
                    if (iy < bottomFadeHeight) {
                        const double bottomFade = static_cast<double>(bottomFadeHeight - iy) / static_cast<double>(bottomFadeHeight - 1);
                        density = density * (1.0 - bottomFade) + -30.0 * bottomFade;
                    }
                    heightMap[static_cast<std::size_t>(index++)] = density;
                }
            }
        }
        return heightMap;
    }

    JavaRandom random_;
    World* world_ = nullptr;
    OctavePerlinNoiseSampler minLimitPerlinNoise_;
    OctavePerlinNoiseSampler maxLimitPerlinNoise_;
    OctavePerlinNoiseSampler perlinNoise1_;
    OctavePerlinNoiseSampler perlinNoise2_;
    OctavePerlinNoiseSampler perlinNoise3_;
    OctavePerlinNoiseSampler floatingIslandScale_;
    OctavePerlinNoiseSampler floatingIslandNoise_;
    OctavePerlinNoiseSampler forestNoise_;
    BiomeSource biomeSource_;
    std::vector<double> heightMap_;
    std::vector<BiomeInfo> biomes_;
    std::vector<double> sandBuffer_;
    std::vector<double> gravelBuffer_;
    std::vector<double> depthBuffer_;
    CaveWorldCarver cave_;
    std::vector<double> perlinNoiseBuffer_;
    std::vector<double> minLimitPerlinNoiseBuffer_;
    std::vector<double> maxLimitPerlinNoiseBuffer_;
    std::vector<double> scaleNoiseBuffer_;
    std::vector<double> depthNoiseBuffer_;
    std::vector<double> decorateTemperatures_;
    std::optional<Chunk> scratchChunk_;
};

} // namespace net::minecraft
