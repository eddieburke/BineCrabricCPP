// world_gen_parity_test.cpp - golden checks traced against beta 1.7.3 Java worldgen.
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/LegacyChunkCache.hpp"
#include "net/minecraft/world/gen/chunk/OverworldGeneratorChunkSource.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

using net::minecraft::BiomeSource;
using net::minecraft::Block;
using net::minecraft::OverworldGeneratorChunkSource;

namespace {

int failures = 0;

void check(bool ok, const char* message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

void checkEqual(int got, int expected, const char* message)
{
    if (got != expected) {
        std::fprintf(stderr, "FAIL: %s got=%d expected=%d\n", message, got, expected);
        ++failures;
    }
}

int topSolidY(const net::minecraft::Chunk& chunk, int x, int z)
{
    for (int y = 127; y >= 0; --y) {
        const int id = chunk.getBlockId(x, y, z);
        if (id != 0 && Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(id)] != 0) {
            return y;
        }
    }
    return -1;
}

struct WorldGenSummary {
    int chunks = 0;
    int populated = 0;
    int grassSurface = 0;
    int sandSurface = 0;
    int gravelSurface = 0;
    int dirtSurface = 0;
    int waterSurface = 0;
    int biomeGrassTop = 0;
    int biomeSandTop = 0;
    int biomeDesert = 0;
    int biomePlains = 0;
    int biomeOther = 0;
    int logs = 0;
    int leaves = 0;
    int tallGrass = 0;
    int flowers = 0;
    int cactus = 0;
    int sugarCane = 0;
    net::minecraft::BiomeId decorateBiome = net::minecraft::BiomeId::Plains;
};

void countTopSurface(const net::minecraft::Chunk& chunk, int x, int z, WorldGenSummary& summary)
{
    const int top = topSolidY(chunk, x, z);
    if (top < 0) {
        return;
    }
    const int id = chunk.getBlockId(x, top, z);
    if (id == Block::GRASS_BLOCK->id) {
        ++summary.grassSurface;
    } else if (id == Block::SAND->id) {
        ++summary.sandSurface;
    } else if (id == Block::GRAVEL->id) {
        ++summary.gravelSurface;
    } else if (id == Block::DIRT->id) {
        ++summary.dirtSurface;
    } else if (id == Block::WATER->id || id == Block::FLOWING_WATER->id) {
        ++summary.waterSurface;
    }
}

void countBiome(const net::minecraft::Biome& biome, WorldGenSummary& summary)
{
    if (biome.topBlockId == Block::SAND->id) {
        ++summary.biomeSandTop;
    } else if (biome.topBlockId == Block::GRASS_BLOCK->id) {
        ++summary.biomeGrassTop;
    }

    if (biome.id == net::minecraft::BiomeId::Desert) {
        ++summary.biomeDesert;
    } else if (biome.id == net::minecraft::BiomeId::Plains) {
        ++summary.biomePlains;
    } else {
        ++summary.biomeOther;
    }
}

void countPopulationBlocks(const net::minecraft::Chunk& chunk, int x, int z, WorldGenSummary& summary)
{
    for (int y = 0; y < 128; ++y) {
        const int id = chunk.getBlockId(x, y, z);
        if (id == Block::LOG->id) {
            ++summary.logs;
        } else if (id == Block::LEAVES->id) {
            ++summary.leaves;
        } else if (id == Block::GRASS->id) {
            ++summary.tallGrass;
        } else if (id == Block::DANDELION->id || id == Block::ROSE->id) {
            ++summary.flowers;
        } else if (id == Block::CACTUS->id) {
            ++summary.cactus;
        } else if (id == Block::SUGAR_CANE->id) {
            ++summary.sugarCane;
        }
    }
}

WorldGenSummary summarizeDirectPath(std::uint64_t seed, int originX, int originZ, int radius)
{
    auto storage = std::make_unique<net::minecraft::EmptyWorldStorage>();
    net::minecraft::World world(storage.get(), "direct-probe", static_cast<std::int64_t>(seed), true);
    OverworldGeneratorChunkSource source(&world, seed);

    WorldGenSummary summary {};
    for (int cz = originZ - radius; cz <= originZ + radius; ++cz) {
        for (int cx = originX - radius; cx <= originX + radius; ++cx) {
            net::minecraft::Chunk& chunk = source.loadChunk(cx, cz);
            std::vector<net::minecraft::Biome*> chunkBiomes;
            if (BiomeSource* biomeSource = world.getBiomeSource()) {
                chunkBiomes = biomeSource->getBiomesInArea(chunkBiomes, cx * 16, cz * 16, 16, 16);
            }
            ++summary.chunks;
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    countTopSurface(chunk, x, z, summary);
                }
            }
        }
    }
    return summary;
}

WorldGenSummary summarizeCachePath(std::uint64_t seed, int originX, int originZ, int radius)
{
    auto storage = std::make_unique<net::minecraft::EmptyWorldStorage>();
    net::minecraft::World world(storage.get(), "cache-probe", static_cast<std::int64_t>(seed), true);
    world.setEventProcessingEnabled(true);
    auto* source = dynamic_cast<net::minecraft::LegacyChunkCache*>(world.getChunkSource());
    check(source != nullptr, "world cache should be LegacyChunkCache");

    WorldGenSummary summary {};
    if (source == nullptr) {
        return summary;
    }

    source->setSpawnPoint(originX, originZ);
    source->setActiveRadius(radius + 2);
    for (int cz = originZ - radius; cz <= originZ + radius + 1; ++cz) {
        for (int cx = originX - radius; cx <= originX + radius + 1; ++cx) {
            (void)world.getBlockId(cx * 16 + 8, 64, cz * 16 + 8);
        }
    }
    for (int cz = originZ - radius; cz <= originZ + radius; ++cz) {
        for (int cx = originX - radius; cx <= originX + radius; ++cx) {
            source->getChunk(cx, cz);
            source->getChunk(cx + 1, cz);
            source->getChunk(cx, cz + 1);
            source->getChunk(cx + 1, cz + 1);
            source->decorate(source, cx, cz);
        }
    }

    if (BiomeSource* biomeSource = world.getBiomeSource()) {
        summary.decorateBiome = biomeSource->getBiome(originX * 16 + 16, originZ * 16 + 16).id;
    }

    for (int cz = originZ - radius; cz <= originZ + radius; ++cz) {
        for (int cx = originX - radius; cx <= originX + radius; ++cx) {
            net::minecraft::Chunk& chunk = source->getChunk(cx, cz);
            std::vector<net::minecraft::Biome*> chunkBiomes;
            if (BiomeSource* biomeSource = world.getBiomeSource()) {
                chunkBiomes = biomeSource->getBiomesInArea(chunkBiomes, cx * 16, cz * 16, 16, 16);
            }
            ++summary.chunks;
            if (chunk.terrainPopulated) {
                ++summary.populated;
            }
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    if (!chunkBiomes.empty()) {
                        countBiome(*chunkBiomes[static_cast<std::size_t>(x + z * 16)], summary);
                    }
                    countTopSurface(chunk, x, z, summary);
                    countPopulationBlocks(chunk, x, z, summary);
                }
            }
        }
    }
    return summary;
}

void checkBiomeRegistryParity()
{
    checkEqual(net::minecraft::Biome::desert().topBlockId, Block::SAND->id, "Desert top block should be sand");
    checkEqual(net::minecraft::Biome::desert().soilBlockId, Block::SAND->id, "Desert soil block should be sand");
    checkEqual(net::minecraft::Biome::iceDesert().topBlockId, Block::SAND->id, "Ice Desert top block should be sand");
    checkEqual(net::minecraft::Biome::iceDesert().soilBlockId, Block::SAND->id, "Ice Desert soil block should be sand");
}

void checkBiomeSourceReseedParity()
{
    BiomeSource reused(0ULL);
    std::vector<net::minecraft::Biome*> reusedArea;
    reused.getBiomesInArea(reusedArea, 0, 0, 16, 16);
    reused.setSeed(404ULL);
    reused.getBiomesInArea(reusedArea, -16, -16, 16, 16);

    BiomeSource fresh(404ULL);
    std::vector<net::minecraft::Biome*> freshArea;
    fresh.getBiomesInArea(freshArea, -16, -16, 16, 16);

    bool sameBiomes = reusedArea.size() >= 256 && freshArea.size() >= 256;
    for (std::size_t i = 0; sameBiomes && i < 256; ++i) {
        sameBiomes = reusedArea[i] != nullptr && freshArea[i] != nullptr
            && reusedArea[i]->id == freshArea[i]->id
            && reusedArea[i]->topBlockId == freshArea[i]->topBlockId
            && reusedArea[i]->soilBlockId == freshArea[i]->soilBlockId;
    }

    const std::vector<double>& reusedTemperatures = reused.temperatureMap();
    const std::vector<double>& freshTemperatures = fresh.temperatureMap();
    const std::vector<double>& reusedDownfall = reused.downfallMap();
    const std::vector<double>& freshDownfall = fresh.downfallMap();
    bool sameClimate = reusedTemperatures.size() >= 256 && freshTemperatures.size() >= 256
        && reusedDownfall.size() >= 256 && freshDownfall.size() >= 256;
    for (std::size_t i = 0; sameClimate && i < 256; ++i) {
        sameClimate = reusedTemperatures[i] == freshTemperatures[i]
            && reusedDownfall[i] == freshDownfall[i];
    }

    check(sameBiomes, "BiomeSource::setSeed should rebuild sampler biome state");
    check(sameClimate, "BiomeSource::setSeed should rebuild sampler climate state");
}

void checkSeed12345ColumnParity()
{
    constexpr std::uint64_t seed = 12345ULL;
    OverworldGeneratorChunkSource source(nullptr, seed, true);
    net::minecraft::Chunk& chunk = source.loadChunk(0, 0);

    BiomeSource biomeSource(seed);
    const net::minecraft::Biome& biome = biomeSource.getBiome(8, 8);
    check(biome.id == net::minecraft::BiomeId::Savanna, "seed 12345 biome should be Savanna");
    checkEqual(chunk.getBlockId(8, 83, 8), Block::GRASS_BLOCK->id, "seed 12345 surface grass at y=83");
    checkEqual(chunk.getBlockId(8, 78, 8), Block::STONE->id, "seed 12345 stone at y=78");
    checkEqual(chunk.getBlockId(8, 3, 8), Block::BEDROCK->id, "seed 12345 bedrock at y=3");
    checkEqual(chunk.getBlockId(8, 30, 8), 0, "seed 12345 cave air at y=30");
    checkEqual(chunk.getBlockId(8, 29, 8), 0, "seed 12345 cave air at y=29");
}

void checkSeed404Parity()
{
    constexpr std::uint64_t seed = 404ULL;
    const WorldGenSummary direct = summarizeDirectPath(seed, 0, 0, 1);
    checkEqual(direct.chunks, 9, "seed 404 direct chunk count");
    checkEqual(direct.grassSurface, 142, "seed 404 direct grass surface count");
    checkEqual(direct.sandSurface, 610, "seed 404 direct sand surface count");
    checkEqual(direct.gravelSurface, 74, "seed 404 direct gravel surface count");
    checkEqual(direct.dirtSurface, 0, "seed 404 direct dirt surface count");
    checkEqual(direct.waterSurface, 1478, "seed 404 direct water surface count");

    const WorldGenSummary cache = summarizeCachePath(seed, 0, 0, 1);
    checkEqual(cache.chunks, 9, "seed 404 cache chunk count");
    checkEqual(cache.populated, 9, "seed 404 cache populated chunk count");
    checkEqual(static_cast<int>(cache.decorateBiome), static_cast<int>(net::minecraft::BiomeId::Desert),
        "seed 404 cache decorate biome");
    checkEqual(cache.grassSurface, 118, "seed 404 cache grass surface count");
    checkEqual(cache.sandSurface, 610, "seed 404 cache sand surface count");
    checkEqual(cache.gravelSurface, 74, "seed 404 cache gravel surface count");
    checkEqual(cache.dirtSurface, 0, "seed 404 cache dirt surface count");
    checkEqual(cache.waterSurface, 1461, "seed 404 cache water surface count");
    checkEqual(cache.biomeGrassTop, 1184, "seed 404 biome grass-top count");
    checkEqual(cache.biomeSandTop, 1120, "seed 404 biome sand-top count");
    checkEqual(cache.biomeDesert, 1120, "seed 404 desert biome count");
    checkEqual(cache.biomePlains, 1069, "seed 404 plains biome count");
    checkEqual(cache.biomeOther, 115, "seed 404 other biome count");
    checkEqual(cache.logs, 9, "seed 404 log population count");
    checkEqual(cache.leaves, 99, "seed 404 leaves population count");
    checkEqual(cache.tallGrass, 6, "seed 404 tall grass population count");
    checkEqual(cache.flowers, 0, "seed 404 flower population count");
    checkEqual(cache.cactus, 2, "seed 404 cactus population count");
    checkEqual(cache.sugarCane, 0, "seed 404 sugar cane population count");
}

} // namespace

int main()
{
    net::minecraft::block::initializeBlocks();
    net::minecraft::registry::Registry::bootstrap();

    checkBiomeRegistryParity();
    checkBiomeSourceReseedParity();
    checkSeed12345ColumnParity();
    checkSeed404Parity();

    if (failures == 0) {
        std::printf("world_gen_parity_test: passed\n");
        return 0;
    }
    std::fprintf(stderr, "world_gen_parity_test: %d failure(s)\n", failures);
    return 1;
}
