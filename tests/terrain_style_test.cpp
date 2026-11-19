#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <utility>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
#include "net/minecraft/world/gen/TerrainStyle.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
#include "support/historical_terrain_samples.hpp"
using namespace net::minecraft;
using world::gen::TerrainStyle;
namespace {
int columnTop(const Chunk& chunk, int x, int z) {
 for(int y = 127; y >= 0; --y) {
  if(Generator::rawBlock(chunk, x, y, z) != 0) {
   return y;
  }
 }
 return -1;
}
Chunk generate(TerrainStyle style, std::uint64_t seed, int chunkX, int chunkZ) {
 OverworldChunkGenerator generator(nullptr, seed);
 generator.useLocalBiomeSource(true);
 generator.enableModGeneration(false);
 generator.setTerrainStyle(style);
 return generator.loadChunk(&generator, chunkX, chunkZ);
}
}
TEST(TerrainStyle, OptionKeysResolvePerDimension) {
 const std::unordered_map<std::string, std::string> options{
     {"terrain_style:0", "beta"}, {"terrain_style:7", "alpha"}, {"terrain_style:-1", "unknown"}};
 EXPECT_EQ(world::gen::terrainStyleFromOptions(options, 0), TerrainStyle::Beta);
 EXPECT_EQ(world::gen::terrainStyleFromOptions(options, 7), TerrainStyle::Alpha);
 EXPECT_EQ(world::gen::terrainStyleFromOptions(options, -1), TerrainStyle::Beta);
 EXPECT_EQ(world::gen::terrainStyleFromOptions(options, 1), TerrainStyle::Beta);
 EXPECT_EQ(world::gen::parseTerrainStyle(world::gen::terrainStyleName(TerrainStyle::Alpha)), TerrainStyle::Alpha);
 EXPECT_EQ(world::gen::parseTerrainStyle(world::gen::terrainStyleName(TerrainStyle::Beta)), TerrainStyle::Beta);
}
TEST(TerrainStyle, AlphaHasNoClimateHeightVariation) {
 for(double temperature = 0.0; temperature <= 1.0; temperature += 0.125) {
  for(double downfall = 0.0; downfall <= 1.0; downfall += 0.125) {
   EXPECT_DOUBLE_EQ(world::gen::terrainClimateFactor(TerrainStyle::Alpha, temperature, downfall), 1.0);
  }
 }
 EXPECT_LT(world::gen::terrainClimateFactor(TerrainStyle::Beta, 0.1, 0.1), 0.1);
 EXPECT_GT(world::gen::terrainClimateFactor(TerrainStyle::Beta, 1.0, 1.0), 0.99);
 const double cold = world::gen::terrainClimateFactor(TerrainStyle::Beta, 0.2, 0.2);
 const double hot = world::gen::terrainClimateFactor(TerrainStyle::Beta, 0.9, 0.9);
 EXPECT_LT(cold, hot);
}
TEST(TerrainStyle, AlphaTerrainAndSurfaceMatchOriginal11201Bytecode) {
 for(const auto& sample : net::minecraft::test::kAlpha11201Samples) {
  SCOPED_TRACE(::testing::Message() << "seed=" << sample.seed << " chunk=" << sample.chunkX << "," << sample.chunkZ);
  OverworldChunkGenerator generator(nullptr, static_cast<std::uint64_t>(sample.seed));
  generator.useLocalBiomeSource(true);
  generator.enableModGeneration(false);
  generator.setTerrainStyle(TerrainStyle::Alpha);
  const Chunk chunk = generator.loadChunk(nullptr, sample.chunkX, sample.chunkZ);
  std::uint64_t hash = 14695981039346656037ULL;
  for(int x = 0; x < 16; ++x) {
   for(int z = 0; z < 16; ++z) {
    for(int y = 0; y < 128; ++y) {
     hash ^= static_cast<std::uint8_t>(Generator::rawBlock(chunk, x, y, z));
     hash *= 1099511628211ULL;
    }
   }
  }
  EXPECT_EQ(hash, sample.fnv64);
  if(const char* directory = std::getenv("BINECRABRIC_ALPHA_DUMP")) {
   const auto path = std::filesystem::path(directory) /
       (std::to_string(sample.seed) + "_" + std::to_string(sample.chunkX) + "_" + std::to_string(sample.chunkZ) + ".bin");
   std::filesystem::create_directories(path.parent_path());
   std::ofstream output(path, std::ios::binary);
   for(int x = 0; x < 16; ++x) {
    for(int z = 0; z < 16; ++z) {
     for(int y = 0; y < 128; ++y) {
      const auto value = static_cast<char>(Generator::rawBlock(chunk, x, y, z));
      output.write(&value, 1);
     }
    }
   }
  }
 }
}
TEST(TerrainStyle, ColumnHeightsAreDeterministicAndInRange) {
 for(const TerrainStyle style : {TerrainStyle::Beta, TerrainStyle::Alpha}) {
  for(const auto& [cx, cz] : std::array<std::pair<int, int>, 3>{{{0, 0}, {-7, 13}, {40, -22}}}) {
   const Chunk first = generate(style, 90210ULL, cx, cz);
   const Chunk second = generate(style, 90210ULL, cx, cz);
   EXPECT_TRUE(first.blocks == second.blocks);
   for(int x = 0; x < 16; ++x) {
    for(int z = 0; z < 16; ++z) {
     const int top = columnTop(first, x, z);
     EXPECT_GE(top, 0);
     EXPECT_LE(top, 127);
    }
   }
  }
 }
}
TEST(TerrainStyle, DifferentStylesShapeDifferentTerrain) {
 const Chunk beta = generate(TerrainStyle::Beta, 4242ULL, 3, 3);
 const Chunk alpha = generate(TerrainStyle::Alpha, 4242ULL, 3, 3);
 EXPECT_FALSE(beta.blocks == alpha.blocks);
}
TEST(TerrainStyle, WorldCreationOptionsSelectBiomeSourceAndTerrain) {
 World alpha("alpha-style", 31337ULL, {{"terrain_style:0", "alpha"}});
 std::set<int> alphaBiomes;
 std::set<int> betaBiomes;
 World beta("beta-style", 31337ULL);
 for(int x = -2048; x <= 2048; x += 256) {
  for(int z = -2048; z <= 2048; z += 256) {
   alphaBiomes.insert(static_cast<int>(alpha.getBiome(x, z).id));
   betaBiomes.insert(static_cast<int>(beta.getBiome(x, z).id));
  }
 }
 EXPECT_EQ(alphaBiomes.size(), 1U);
 EXPECT_GT(betaBiomes.size(), 2U);
}
