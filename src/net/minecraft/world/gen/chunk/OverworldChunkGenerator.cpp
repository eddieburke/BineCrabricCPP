#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"

#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/BiomeTreeFeature.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/gen/feature/CactusPatchFeature.hpp"
#include "net/minecraft/world/gen/feature/ClayOreFeature.hpp"
#include "net/minecraft/world/gen/feature/DeadBushPatchFeature.hpp"
#include "net/minecraft/world/gen/feature/DungeonFeature.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/GrassPatchFeature.hpp"
#include "net/minecraft/world/gen/feature/LakeFeature.hpp"
#include "net/minecraft/world/gen/feature/OreFeature.hpp"
#include "net/minecraft/world/gen/feature/PlantPatchFeature.hpp"
#include "net/minecraft/world/gen/feature/PumpkinPatchFeature.hpp"
#include "net/minecraft/world/gen/feature/SpringFeature.hpp"
#include "net/minecraft/world/gen/feature/SugarCanePatchFeature.hpp"

#include <algorithm>
#include <memory>

namespace net::minecraft {

BiomeSource* OverworldChunkGenerator::activeBiomeSource()
{
    if (world_ != nullptr && world_->getBiomeSource() != nullptr) {
        return world_->getBiomeSource();
    }
    return &biomeSource_;
}

const BiomeSource* OverworldChunkGenerator::activeBiomeSource() const
{
    if (world_ != nullptr && world_->getBiomeSource() != nullptr) {
        return world_->getBiomeSource();
    }
    return &biomeSource_;
}

Chunk OverworldChunkGenerator::loadChunk(ChunkSource* source, int chunkX, int chunkZ)
{
    random_.setSeed(static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkX) * 341873128712LL + static_cast<std::int64_t>(chunkZ) * 132897987541LL));
    Chunk chunk(world_, chunkX, chunkZ);
    BiomeSource* biomeSource = activeBiomeSource();
    biomes_ = biomeSource->getBiomesInArea(biomes_, chunkX * 16, chunkZ * 16, 16, 16);
    buildTerrain(chunkX, chunkZ, chunk, biomeSource->temperatureMap());
    buildSurfaces(chunkX, chunkZ, chunk, biomes_);
    if (source != nullptr) {
        cave_.place(source, world_, chunkX, chunkZ, chunk);
    }
    if (world_ != nullptr) {
        chunk.populateHeightMap();
    }
    return chunk;
}

void OverworldChunkGenerator::decorate(ChunkSource* /*source*/, int chunkX, int chunkZ)
{
    block::FallingBlock::fallInstantly = true;
    const int blockOriginX = chunkX * 16;
    const int blockOriginZ = chunkZ * 16;
    BiomeSource* biomeSource = activeBiomeSource();
    const BiomeInfo biome = biomeSource->getBiome(blockOriginX + 16, blockOriginZ + 16);
    random_.setSeed(world_->getSeed());
    const std::int64_t l = random_.nextLong() / 2LL * 2LL + 1LL;
    const std::int64_t l2 = random_.nextLong() / 2LL * 2LL + 1LL;
    random_.setSeed(static_cast<std::uint64_t>((static_cast<std::int64_t>(chunkX) * l + static_cast<std::int64_t>(chunkZ) * l2)
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

    constexpr double forestScale = 0.25;
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

    int flowerPatches = 0;
    if (biome.id == BiomeId::Forest) {
        flowerPatches = 2;
    } else if (biome.id == BiomeId::SeasonalForest) {
        flowerPatches = 4;
    } else if (biome.id == BiomeId::Taiga) {
        flowerPatches = 2;
    } else if (biome.id == BiomeId::Plains) {
        flowerPatches = 3;
    }
    for (int i = 0; i < flowerPatches; ++i) {
        PlantPatchFeature(Block::DANDELION->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
            blockOriginZ + random_.nextInt(16) + 8);
    }

    int grassPatches = 0;
    if (biome.id == BiomeId::Forest || biome.id == BiomeId::SeasonalForest) {
        grassPatches = 2;
    } else if (biome.id == BiomeId::Rainforest || biome.id == BiomeId::Plains) {
        grassPatches = 10;
    } else if (biome.id == BiomeId::Taiga) {
        grassPatches = 1;
    }
    if (Block::GRASS != nullptr) {
        for (int i = 0; i < grassPatches; ++i) {
            int grassMeta = 1;
            if (biome.id == BiomeId::Rainforest && random_.nextInt(3) != 0) {
                grassMeta = 2;
            }
            GrassPatchFeature(Block::GRASS->id, grassMeta).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
                blockOriginZ + random_.nextInt(16) + 8);
        }
    }

    const int deadBushPatches = biome.id == BiomeId::Desert ? 2 : 0;
    for (int i = 0; i < deadBushPatches; ++i) {
        DeadBushPatchFeature(Block::DEAD_BUSH->id).generate(world_, random_, blockOriginX + random_.nextInt(16) + 8, random_.nextInt(128),
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
    const int cactusPatches = biome.id == BiomeId::Desert ? 10 : 0;
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

std::vector<double>& OverworldChunkGenerator::generateHeightMap(std::vector<double>& heightMap, int x, int y, int z, int sizeX, int sizeY, int sizeZ)
{
    const std::size_t size = static_cast<std::size_t>(sizeX * sizeY * sizeZ);
    if (heightMap.size() < size) {
        heightMap.assign(size, 0.0);
    }
    constexpr double baseScale = 684.412;
    const BiomeSource* biomeSource = activeBiomeSource();
    const std::vector<double>& temperatureMap = biomeSource->temperatureMap();
    const std::vector<double>& downfallMap = biomeSource->downfallMap();
    scaleNoiseBuffer_ = floatingIslandScale_.create(scaleNoiseBuffer_, x, z, sizeX, sizeZ, 1.121, 1.121, 0.5);
    depthNoiseBuffer_ = floatingIslandNoise_.create(depthNoiseBuffer_, x, z, sizeX, sizeZ, 200.0, 200.0, 0.5);
    perlinNoiseBuffer_ = perlinNoise1_.create(perlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale / 80.0, baseScale / 160.0, baseScale / 80.0);
    minLimitPerlinNoiseBuffer_ = minLimitPerlinNoise_.create(minLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, baseScale, baseScale);
    maxLimitPerlinNoiseBuffer_ = maxLimitPerlinNoise_.create(maxLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, baseScale, baseScale);

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
            if (depthNoise < 0.0) {
                depthNoise = std::max(depthNoise / 2.0, -1.0);
                depthNoise /= 1.4;
                depthNoise /= 2.0;
                scaleNoise = 0.0;
            } else {
                depthNoise = std::min(depthNoise, 1.0) / 8.0;
            }
            scaleNoise = std::max(scaleNoise, 0.0) + 0.5;
            depthNoise = depthNoise * static_cast<double>(sizeY) / 16.0;
            const double center = static_cast<double>(sizeY) / 2.0 + depthNoise * 4.0;
            ++horizontalIndex;

            for (int iy = 0; iy < sizeY; ++iy) {
                double vertical = (static_cast<double>(iy) - center) * 12.0 / scaleNoise;
                if (vertical < 0.0) {
                    vertical *= 4.0;
                }
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

} // namespace net::minecraft
