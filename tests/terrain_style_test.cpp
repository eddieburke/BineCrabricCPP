#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <set>
#include <utility>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
#include "net/minecraft/world/gen/ModernTerrain.hpp"
#include "net/minecraft/world/gen/TerrainStyle.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
using namespace net::minecraft;
using world::gen::ModernTerrain;
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
int terrainTop(const Chunk& chunk, int x, int z) {
 for(int y = 127; y >= 0; --y) {
  const int id = Generator::rawBlock(chunk, x, y, z);
  if(id != 0 && id != Block::WATER->id && id != Block::ICE->id) {
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
     {"terrain_style:0", "modern"}, {"terrain_style:7", "alpha"}, {"terrain_style:-1", "unknown"}};
 EXPECT_EQ(world::gen::terrainStyleFromOptions(options, 0), TerrainStyle::Modern);
 EXPECT_EQ(world::gen::terrainStyleFromOptions(options, 7), TerrainStyle::Alpha);
 EXPECT_EQ(world::gen::terrainStyleFromOptions(options, -1), TerrainStyle::Beta);
 EXPECT_EQ(world::gen::terrainStyleFromOptions(options, 1), TerrainStyle::Beta);
 EXPECT_EQ(world::gen::parseTerrainStyle(world::gen::terrainStyleName(TerrainStyle::Alpha)), TerrainStyle::Alpha);
 EXPECT_EQ(world::gen::parseTerrainStyle(world::gen::terrainStyleName(TerrainStyle::Modern)), TerrainStyle::Modern);
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
TEST(TerrainStyle, ColumnHeightsAreDeterministicAndInRange) {
 for(const TerrainStyle style : {TerrainStyle::Beta, TerrainStyle::Alpha, TerrainStyle::Modern}) {
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
 const Chunk modern = generate(TerrainStyle::Modern, 4242ULL, 3, 3);
 EXPECT_FALSE(modern.blocks == beta.blocks);
 EXPECT_FALSE(modern.blocks == alpha.blocks);
}
TEST(TerrainStyle, ModernCurveIsMonotonicAndBounded) {
 double previous = -1.0;
 for(double c = -1.0; c <= 1.0; c += 0.01) {
  const double height = ModernTerrain::baseHeight(c);
  EXPECT_GE(height, previous - 1e-9);
  previous = height;
 }
 EXPECT_LT(ModernTerrain::baseHeight(-1.0), 32.0);
 EXPECT_GT(ModernTerrain::baseHeight(1.0), 80.0);
 for(double c = -1.0; c <= 1.0; c += 0.1) {
  for(double e = -1.0; e <= 1.0; e += 0.2) {
   for(double w = -1.0; w <= 1.0; w += 0.1) {
    for(double r = -1.0; r <= 1.0; r += 0.1) {
     const double height = ModernTerrain::surfaceHeight({c, e, w, r});
     EXPECT_GE(height, 20.0);
     EXPECT_LE(height, 116.0);
    }
   }
  }
 }
}
TEST(TerrainStyle, ModernProducesMountainsOceansAndRivers) {
 int highest = 0;
 int lowest = 127;
 int oceanColumns = 0;
 int columns = 0;
 std::set<int> distinctTops;
 for(int cx = -48; cx < 48; cx += 6) {
  for(int cz = -48; cz < 48; cz += 6) {
   const Chunk chunk = generate(TerrainStyle::Modern, 777ULL, cx, cz);
   for(int x = 0; x < 16; x += 5) {
    for(int z = 0; z < 16; z += 5) {
     const int top = terrainTop(chunk, x, z);
     highest = std::max(highest, top);
     lowest = std::min(lowest, top);
     oceanColumns += top < 62 ? 1 : 0;
     ++columns;
     distinctTops.insert(top);
    }
   }
  }
 }
 EXPECT_GE(highest, 95);
 EXPECT_LE(lowest, 45);
 EXPECT_LE(highest, 127);
 EXPECT_GT(oceanColumns, columns / 20);
 EXPECT_LT(oceanColumns, columns * 3 / 5);
 EXPECT_GT(distinctTops.size(), 40U);
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
