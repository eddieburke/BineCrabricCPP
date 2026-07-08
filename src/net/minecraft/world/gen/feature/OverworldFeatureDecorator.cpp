#include "net/minecraft/world/gen/feature/OverworldFeatureDecorator.hpp"

#include <memory>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
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

namespace net::minecraft::world::gen::feature {
void decorateOverworldChunk(World* world,
                            JavaRandom& random,
                            BiomeSource& biomeSource,
                            OctavePerlinNoiseSampler& forestNoise,
                            int chunkX,
                            int chunkZ,
                            std::vector<double>& decorateTemperatures) {
    const int blockOriginX = chunkX * 16;
    const int blockOriginZ = chunkZ * 16;
    const Biome& biome = biomeSource.getBiome(blockOriginX + 16, blockOriginZ + 16);
    if (random.nextInt(4) == 0) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        LakeFeature(Block::WATER->id).generate(world, random, featureX, featureY, featureZ);
    }
    if (random.nextInt(8) == 0) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int lakeY = random.nextInt(random.nextInt(120) + 8);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        if (lakeY < 64 || random.nextInt(10) == 0) {
            LakeFeature(Block::LAVA->id).generate(world, random, featureX, lakeY, featureZ);
        }
    }
    for (int i = 0; i < 8; ++i) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        DungeonFeature().generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 10; ++i) {
        const int featureX = blockOriginX + random.nextInt(16);
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16);
        ClayOreFeature(32).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 20; ++i) {
        const int featureX = blockOriginX + random.nextInt(16);
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16);
        OreFeature(Block::DIRT->id, 32).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 10; ++i) {
        const int featureX = blockOriginX + random.nextInt(16);
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16);
        OreFeature(Block::GRAVEL->id, 32).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 20; ++i) {
        const int featureX = blockOriginX + random.nextInt(16);
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16);
        OreFeature(Block::COAL_ORE->id, 16).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 20; ++i) {
        const int featureX = blockOriginX + random.nextInt(16);
        const int featureY = random.nextInt(64);
        const int featureZ = blockOriginZ + random.nextInt(16);
        OreFeature(Block::IRON_ORE->id, 8).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 2; ++i) {
        const int featureX = blockOriginX + random.nextInt(16);
        const int featureY = random.nextInt(32);
        const int featureZ = blockOriginZ + random.nextInt(16);
        OreFeature(Block::GOLD_ORE->id, 8).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 8; ++i) {
        const int featureX = blockOriginX + random.nextInt(16);
        const int featureY = random.nextInt(16);
        const int featureZ = blockOriginZ + random.nextInt(16);
        OreFeature(Block::REDSTONE_ORE->id, 7).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 1; ++i) {
        const int featureX = blockOriginX + random.nextInt(16);
        const int featureY = random.nextInt(16);
        const int featureZ = blockOriginZ + random.nextInt(16);
        OreFeature(Block::DIAMOND_ORE->id, 7).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 1; ++i) {
        const int featureX = blockOriginX + random.nextInt(16);
        const int firstY = random.nextInt(16);
        const int featureY = firstY + random.nextInt(16);
        const int featureZ = blockOriginZ + random.nextInt(16);
        OreFeature(Block::LAPIS_ORE->id, 6).generate(world, random, featureX, featureY, featureZ);
    }
    constexpr double forestScale = 0.5;
    const int forestRoll = static_cast<int>((forestNoise.sample(static_cast<double>(blockOriginX) * forestScale,
                                                                static_cast<double>(blockOriginZ) * forestScale) /
                                                 8.0 +
                                             random.nextDouble() * 4.0 + 4.0) /
                                            3.0);
    int trees = 0;
    if (random.nextInt(10) == 0) {
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
        const int treeX = blockOriginX + random.nextInt(16) + 8;
        const int treeZ = blockOriginZ + random.nextInt(16) + 8;
        std::unique_ptr<Feature> feature = biome.getRandomTreeFeature(random);
        feature->prepare(1.0, 1.0, 1.0);
        const int treeY = world->getTopY(treeX, treeZ);
        feature->generate(world, random, treeX, treeY, treeZ);
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
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        PlantPatchFeature(Block::DANDELION->id).generate(world, random, featureX, featureY, featureZ);
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
            if (biome.id == BiomeId::Rainforest && random.nextInt(3) != 0) {
                grassMeta = 2;
            }
            const int featureX = blockOriginX + random.nextInt(16) + 8;
            const int featureY = random.nextInt(128);
            const int featureZ = blockOriginZ + random.nextInt(16) + 8;
            GrassPatchFeature(Block::GRASS->id, grassMeta).generate(world, random, featureX, featureY, featureZ);
        }
    }
    const int deadBushPatches = biome.id == BiomeId::Desert ? 2 : 0;
    for (int i = 0; i < deadBushPatches; ++i) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        DeadBushPatchFeature(Block::DEAD_BUSH->id).generate(world, random, featureX, featureY, featureZ);
    }
    if (random.nextInt(2) == 0) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        PlantPatchFeature(Block::ROSE->id).generate(world, random, featureX, featureY, featureZ);
    }
    if (random.nextInt(4) == 0) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        PlantPatchFeature(Block::BROWN_MUSHROOM->id).generate(world, random, featureX, featureY, featureZ);
    }
    if (random.nextInt(8) == 0) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        PlantPatchFeature(Block::RED_MUSHROOM->id).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 10; ++i) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        SugarCanePatchFeature().generate(world, random, featureX, featureY, featureZ);
    }
    if (random.nextInt(32) == 0) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        PumpkinPatchFeature().generate(world, random, featureX, featureY, featureZ);
    }
    const int cactusPatches = biome.id == BiomeId::Desert ? 10 : 0;
    for (int i = 0; i < cactusPatches; ++i) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(128);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        CactusPatchFeature().generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 50; ++i) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(random.nextInt(120) + 8);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        SpringFeature(Block::FLOWING_WATER->id).generate(world, random, featureX, featureY, featureZ);
    }
    for (int i = 0; i < 20; ++i) {
        const int featureX = blockOriginX + random.nextInt(16) + 8;
        const int featureY = random.nextInt(random.nextInt(random.nextInt(112) + 8) + 8);
        const int featureZ = blockOriginZ + random.nextInt(16) + 8;
        SpringFeature(Block::FLOWING_LAVA->id).generate(world, random, featureX, featureY, featureZ);
    }
    decorateTemperatures = biomeSource.create(decorateTemperatures, blockOriginX + 8, blockOriginZ + 8, 16, 16);
    for (int wx = blockOriginX + 8; wx < blockOriginX + 8 + 16; ++wx) {
        for (int wz = blockOriginZ + 8; wz < blockOriginZ + 8 + 16; ++wz) {
            const int localX = wx - (blockOriginX + 8);
            const int localZ = wz - (blockOriginZ + 8);
            const int topSolidY = world->getTopSolidBlockY(wx, wz);
            const double temperature = decorateTemperatures[static_cast<std::size_t>(localX * 16 + localZ)] -
                                       static_cast<double>(topSolidY - 64) / 64.0 * 0.3;
            if (temperature >= 0.5 || topSolidY <= 0 || topSolidY >= 128 || !world->isAir(wx, topSolidY, wz)) {
                continue;
            }
            block::material::Material& groundMaterial = world->getMaterial(wx, topSolidY - 1, wz);
            if (!groundMaterial.blocksMovement() || &groundMaterial == &block::material::Material::ICE) {
                continue;
            }
            world->setBlock(wx, topSolidY, wz, Block::SNOW->id);
        }
    }
}
}  // namespace net::minecraft::world::gen::feature
