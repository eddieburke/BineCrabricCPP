// world_gen_parity_test.cpp — compare chunk (0,0) column (8,8) against Java reference output.
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/gen/chunk/OverworldGeneratorChunkSource.hpp"

#include <cstdio>
#include <cstdlib>

using net::minecraft::BiomeSource;
using net::minecraft::Block;
using net::minecraft::OverworldGeneratorChunkSource;

static int failures = 0;

static void check(bool ok, const char* message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

int main(int argc, char** argv)
{
    net::minecraft::block::initializeBlocks();
    net::minecraft::registry::Registry::bootstrap();
    const std::uint64_t seed = argc > 1 ? static_cast<std::uint64_t>(std::stoull(argv[1])) : 12345ULL;

    OverworldGeneratorChunkSource source(nullptr, seed, true);
    net::minecraft::Chunk& chunk = source.loadChunk(0, 0);

    BiomeSource biomeSource(seed);
    const net::minecraft::BiomeInfo biome = biomeSource.getBiome(8, 8);

    std::printf("seed=%llu\n", static_cast<unsigned long long>(seed));
    std::printf("biome=%d\n", static_cast<int>(biome.id));
    std::printf("STONE=%d GRASS_BLOCK=%d GRASS=%d\n",
        Block::STONE != nullptr ? Block::STONE->id : -1,
        Block::GRASS_BLOCK != nullptr ? Block::GRASS_BLOCK->id : -1,
        Block::GRASS != nullptr ? Block::GRASS->id : -1);

    int undergroundAir = 0;
    for (int y = 0; y < 80; ++y) {
        if (chunk.getBlockId(8, y, 8) == 0) {
            ++undergroundAir;
        }
    }
    std::printf("underground_air_below_80=%d\n", undergroundAir);

    for (int y = 127; y >= 0; --y) {
        const int id = chunk.getBlockId(8, y, 8);
        if (id != 0) {
            std::printf("y=%d block=%d\n", y, id);
        }
    }

    if (seed == 12345ULL) {
        check(biome.id == net::minecraft::BiomeId::Savanna, "biome should be Savanna for seed 12345");
        check(chunk.getBlockId(8, 83, 8) == Block::GRASS_BLOCK->id, "surface grass block at y=83");
        check(chunk.getBlockId(8, 78, 8) == Block::STONE->id, "stone at y=78");
        check(chunk.getBlockId(8, 3, 8) == Block::BEDROCK->id, "bedrock at y=3");
        check(chunk.getBlockId(8, 30, 8) == 0, "cave air at y=30");
        check(chunk.getBlockId(8, 29, 8) == 0, "cave air at y=29");
    }

    if (failures == 0) {
        std::printf("world_gen_parity_test: passed\n");
        return 0;
    }
    std::fprintf(stderr, "world_gen_parity_test: %d failure(s)\n", failures);
    return 1;
}
