#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include "net/minecraft/client/color/world/GrassColors.hpp"
#include "net/minecraft/client/color/world/FoliageColors.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/distant/DistantTile.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::test {
namespace {
namespace option = client::option;
using namespace client::render::distant;
struct LodColumns {
 std::array<std::unique_ptr<Chunk>, 36> chunks;
 LodColumns() {
  for(int z = 0; z < 6; ++z) {
   for(int x = 0; x < 6; ++x) {
    auto& chunk = chunks[static_cast<std::size_t>(z * 6 + x)];
    chunk = std::make_unique<Chunk>(nullptr, x - 1, z - 1);
    for(int cz = 0; cz < 16; ++cz) {
     for(int cx = 0; cx < 16; ++cx) {
      for(int y = 0; y < 64; ++y)
       chunk->blocks[(cx << 11) | (cz << 7) | y] = y == 63 ? 2 : y >= 60 ? 3 : 1;
      chunk->heightmap[(cz << 4) | cx] = 64;
     }
    }
   }
  }
 }
 ChunkLookup lookup() const {
  return [this](int x, int z) -> const Chunk* {
   if(x < -1 || x > 4 || z < -1 || z > 4) return nullptr;
   return chunks[static_cast<std::size_t>((z + 1) * 6 + x + 1)].get();
  };
 }
};
class LodClimate : public BiomeSource {
 public:
 LodClimate() : BiomeSource(1) {}
 ClimateSample sampleClimate(int x, int) const override {
  return x < 32 ? ClimateSample{0.2, 0.8} : ClimateSample{0.9, 0.1};
 }
};
}
TEST(LodRegression, OneSwitchControlsTheOnlyLodRenderer) {
 option::GameOptions options;
 options.renderScale = 3.0f;
 auto settings = option::renderSettings(options);
 EXPECT_TRUE(settings.distantTerrain);
 EXPECT_EQ(settings.nativeChunkRadius, 16);
 options.distantTerrain = false;
 settings = option::renderSettings(options);
 EXPECT_FALSE(settings.distantTerrain);
 EXPECT_EQ(settings.nativeChunkRadius, 48);
 EXPECT_FALSE(option::OptionRegistry::byKey("terrainLod").has_value());
}
TEST(LodRegression, NativeSettingsPersistThroughRegistry) {
 option::GameOptions options;
 for(const auto& [key, value] : std::array<std::pair<const char*, const char*>, 4>{{
         {"distantTerrain", "false"}, {"lodNativeRadius", "24"}, {"lodQuality", "2"},
         {"lodMemoryMiB", "128"}}}) {
  const auto spec = option::OptionRegistry::byKey(key);
  ASSERT_TRUE(spec.has_value()) << key;
  (*spec)->load(options, value);
  std::ostringstream saved;
  (*spec)->save(options, saved);
  EXPECT_EQ(saved.str(), value) << key;
 }
 EXPECT_FALSE(options.distantTerrain);
 EXPECT_EQ(options.lodNativeRadius, 24);
 EXPECT_EQ(options.lodQuality, 2);
 EXPECT_EQ(options.lodMemoryMiB, 128);
 EXPECT_FALSE(option::OptionRegistry::byKey("lodBuildRate").has_value());
}
TEST(LodRegression, ControlsFollowTheSwitchWithoutLockingTheRadiusAtShortViewDistances) {
 option::GameOptions options;
 options.viewDistance = 3;
 EXPECT_FALSE(option::renderSettings(options).distantTerrain);
 for(const auto key : {"lodNativeRadius", "lodQuality", "lodMemoryMiB"}) {
  const auto spec = option::OptionRegistry::byKey(key);
  ASSERT_TRUE(spec.has_value());
  ASSERT_NE((*spec)->isEnabled, nullptr);
  EXPECT_EQ((*spec)->isEnabled(options), std::string_view(key) == "lodNativeRadius");
  options.distantTerrain = false;
  EXPECT_FALSE((*spec)->isEnabled(options));
  options.distantTerrain = true;
  options.viewDistance = 0;
  options.renderScale = 3.0f;
  EXPECT_TRUE((*spec)->isEnabled(options));
  options.viewDistance = 3;
  options.renderScale = 1.0f;
 }
}
TEST(LodRegression, RectangularBiomeQueriesMatchIndependentClimateSamples) {
 BiomeSource source(12345ULL);
 std::vector<Biome*> biomes;
 for(const auto [width, depth] : {std::pair{2, 7}, std::pair{7, 2}, std::pair{1, 9}}) {
  source.getBiomesInArea(biomes, -17, 33, width, depth);
  ASSERT_GE(biomes.size(), static_cast<std::size_t>(width * depth));
  for(int x = 0; x < width; ++x) {
   for(int z = 0; z < depth; ++z) {
    const auto climate = source.sampleClimate(-17 + x, 33 + z);
    EXPECT_EQ(biomes[static_cast<std::size_t>(x * depth + z)],
              &Biome::getBiome(climate.temperature, climate.downfall));
   }
  }
 }
}
TEST(LodRegression, LegacySwitchMigratesAndExplicitSwitchWinsInEitherOrder) {
 struct OptionsFile {
  std::filesystem::path path = std::filesystem::temp_directory_path() /
      ("bine_lod_options_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt");
  ~OptionsFile() {
   std::error_code error;
   std::filesystem::remove(path, error);
  }
 } file;
 for(const auto contents : {"terrainLod:false\n", "terrainLod:true\ndistantTerrain:false\n",
                             "distantTerrain:false\nterrainLod:true\n"}) {
  {
   std::ofstream output(file.path);
   output << contents;
  }
  option::GameOptions options;
  options.optionsFile = file.path;
  options.load();
  EXPECT_FALSE(options.distantTerrain);
  options.save();
  std::ifstream input(file.path);
  std::ostringstream saved;
  saved << input.rdbuf();
  EXPECT_EQ(saved.str().find("terrainLod:"), std::string::npos);
  EXPECT_NE(saved.str().find("distantTerrain:false"), std::string::npos);
 }
}
TEST(LodRegression, InvalidSettingsAreBoundedAndUnsupportedPacksRetainNativeCoverage) {
 option::GameOptions options;
 options.renderScale = 5.0f;
 options.lodNativeRadius = -10;
 options.lodQuality = 100;
 options.lodMemoryMiB = std::numeric_limits<int>::max();
 auto settings = option::renderSettings(options);
 EXPECT_EQ(settings.nativeChunkRadius, 8);
 EXPECT_EQ(settings.lodQuality, 2);
 EXPECT_EQ(settings.lodMemoryMiB, 512);
 client::render::PackDefinition pack;
 pack.programs["gbuffers_terrain"] = {};
 option::applyShaderPack(settings, pack);
 EXPECT_FALSE(settings.distantTerrain);
 EXPECT_EQ(settings.nativeChunkRadius, 80);
 pack.programs["dh_water"] = {};
 EXPECT_FALSE(option::supportsDistantTerrain(pack));
 pack.programs["dh_terrain"] = {};
 EXPECT_TRUE(option::supportsDistantTerrain(pack));
 pack.voxelizeLightBlocks = true;
 settings = option::renderSettings(options, pack);
 EXPECT_FALSE(settings.distantTerrain);
 EXPECT_EQ(settings.nativeChunkRadius, 80);
}
TEST(LodRegression, ProviderFlagOverridesCachedProgramAvailability) {
 client::render::PackDefinition pack;
 client::render::ProgramEnabledCache cache;
 EXPECT_FALSE(client::render::isProgramEnabledCached(pack, {}, "dh_terrain", cache));
 pack.distantHorizons = true;
 EXPECT_TRUE(client::render::isProgramEnabledCached(pack, {}, "dh_terrain", cache));
 pack.distantHorizons = false;
 EXPECT_FALSE(client::render::isProgramEnabledCached(pack, {}, "dh_terrain", cache));
 EXPECT_TRUE(client::render::isProgramEnabledCached(pack, {}, "gbuffers_terrain", cache));
}
TEST(LodRegression, CliffRetainsGrassDirtAndStoneLayers) {
 LodColumns grid;
 TileCells cells;
 ASSERT_TRUE(extractTile(grid.lookup(), 0, 0, 1, cells));
 const auto& run = cells.at(0, 0).runs[0];
 ASSERT_EQ(run.layerCount, 3);
 EXPECT_EQ(run.layers[0].bottom, 63);
 EXPECT_EQ(run.layers[0].block, 2);
 EXPECT_EQ(run.layers[1].bottom, 60);
 EXPECT_EQ(run.layers[1].block, 3);
 EXPECT_EQ(run.layers[2].bottom, 0);
 EXPECT_EQ(run.layers[2].block, 1);
 cells.at(-1, 0).runs[0].top = 50;
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 std::array<int, 3> areas{};
 for(std::size_t i = 0; i < mesh.opaque.size(); i += 4) {
  const auto* q = &mesh.opaque[i];
  if(q[0].normal != DhNormal::NegX || q[0].x != 0 || q[0].z > 1) continue;
  int low = 128;
  int high = 0;
  for(int v = 0; v < 4; ++v) {
   low = std::min<int>(low, q[v].y);
   high = std::max<int>(high, q[v].y);
  }
  if(q[0].material == DhMaterial::Grass) areas[0] += high - low;
  if(q[0].material == DhMaterial::Dirt) areas[1] += high - low;
  if(q[0].material == DhMaterial::Stone) areas[2] += high - low;
 }
 EXPECT_EQ(areas, (std::array<int, 3>{1, 3, 10}));
}
TEST(LodRegression, ClimateTintReachesCellsAndStopsMergesAcrossBiomeColors) {
 LodColumns grid;
 LodClimate climate;
 TileCells cells;
 ASSERT_TRUE(extractTile(grid.lookup(), 0, 0, 1, cells, &climate));
 for(const int x : {0, 63}) {
  const auto sample = climate.sampleClimate(x, 0);
  const int grass = client::color::world::GrassColors::getColor(sample.temperature, sample.downfall);
  const int foliage = client::color::world::FoliageColors::getColor(sample.temperature, sample.downfall);
  const auto& cell = cells.at(x, 0);
  for(int channel = 0; channel < 3; ++channel) {
   EXPECT_EQ(cell.grass[channel], (grass >> (16 - 8 * channel)) & 255);
   EXPECT_EQ(cell.foliage[channel], (foliage >> (16 - 8 * channel)) & 255);
  }
 }
 for(int z = -1; z <= cells.side; ++z) {
  for(int x = -1; x <= cells.side; ++x) {
   auto& cell = cells.at(x, z);
   cell.grass[0] = x < 32 ? 255 : 0;
   cell.grass[1] = x < 32 ? 0 : 255;
   cell.grass[2] = 0;
  }
 }
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 EXPECT_EQ(mesh.opaque.size(), 64u * 2u * 4u);
 for(const auto& vertex : mesh.opaque) {
  EXPECT_EQ(vertex.color[2], 0);
  EXPECT_TRUE((vertex.color[0] == 150 && vertex.color[1] == 0) ||
              (vertex.color[0] == 0 && vertex.color[1] == 150));
 }
}
TEST(LodRegression, AtlasAveragesReplaceHardcodedColorsWithoutChangingTintOrMaterial) {
 client::texture::RasterImage atlas;
 atlas.width = atlas.height = 256;
 atlas.argb.assign(256u * 256u, 0xFF204060u);
 auto palette = defaultPalette();
 updatePaletteFromAtlas(palette, atlas);
 EXPECT_EQ(palette.entries[1].top[0], 32);
 EXPECT_EQ(palette.entries[1].top[1], 64);
 EXPECT_EQ(palette.entries[1].top[2], 96);
 EXPECT_EQ(palette.entries[2].tint, PaletteTint::GrassTint);
 EXPECT_EQ(palette.entries[18].tint, PaletteTint::FoliageTint);
 EXPECT_EQ(palette.entries[1].material, DhMaterial::Stone);
 EXPECT_EQ(palette.entries[9].top[0], 32);
 for(int y = 0; y < 16; ++y)
  for(int x = 0; x < 16; ++x) atlas.argb[static_cast<std::size_t>(y * 256 + x)] = 0xFFB0B0B0u;
 updatePaletteFromAtlas(palette, atlas);
 EXPECT_EQ(palette.entries[2].top[0], 176);
 EXPECT_EQ(palette.entries[2].side[0], 32);
 atlas.argb.assign(256u * 256u, 0x00FF0000u);
 updatePaletteFromAtlas(palette, atlas);
 EXPECT_EQ(palette.entries[1].top[0], 32);
}
TEST(LodRegression, InvalidStridesFailBeforeReadingChunks) {
 TileCells cells;
 const ChunkLookup lookup = [](int, int) -> const Chunk* { ADD_FAILURE(); return nullptr; };
 for(const int stride : {0, -1, 3, 32}) EXPECT_FALSE(extractTile(lookup, 0, 0, stride, cells));
}
TEST(LodRegression, PartialTilesKeepReadyColumnsAndSkirtOnlyMissingCells) {
 LodColumns grid;
 grid.chunks[7].reset();
 TileCells cells;
 ASSERT_FALSE(extractTile(grid.lookup(), 0, 0, 1, cells));
 ASSERT_TRUE(extractTile(grid.lookup(), 0, 0, 1, cells, nullptr, true));
 EXPECT_EQ(cells.chunkMask, 0xFFFEu);
 EXPECT_FALSE(cells.at(0, 0).present);
 EXPECT_FALSE(cells.at(15, 15).present);
 EXPECT_TRUE(cells.at(16, 0).present);
 EXPECT_TRUE(cells.at(0, 16).present);
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 ASSERT_FALSE(mesh.opaque.empty());
 bool skirt = false;
 for(const auto& vertex : mesh.opaque) {
  if(vertex.x == 16 && vertex.normal == DhNormal::NegX && vertex.y == 0) skirt = true;
 }
 EXPECT_TRUE(skirt);
}
TEST(LodRegression, PressurePreservesNearbyDetailAtTheReportedHighSettings) {
 for(const double nearest : {320.0, 384.0, 448.0, 512.0, 640.0, 767.99})
  EXPECT_EQ(tileStride(nearest, 384.0, 0, 16), 1) << nearest;
 EXPECT_EQ(tileStride(768.0, 384.0, 0, 16), 2);
 EXPECT_EQ(tileStride(1152.0, 384.0, 0, 16), 4);
 EXPECT_EQ(tileStride(1280.0, 384.0, 0, 16), 4);
 EXPECT_EQ(tileStride(1280.0, 384.0, 0, 1), 1);
}
TEST(LodRegression, PressureAndQualityRemainOrderedAcrossDistanceBands) {
 for(const int radius : {128, 256, 384, 512}) {
  for(int pressure = 1; pressure <= 16; pressure *= 2) {
   int previous[3] = {1, 1, 1};
   for(int distance = 0; distance <= 4096; ++distance) {
    int strides[3]{};
    for(int quality = 0; quality < 3; ++quality) {
     const int stride = tileStride(distance, radius, quality, pressure);
     ASSERT_TRUE(stride == 1 || stride == 2 || stride == 4 || stride == 8 || stride == 16);
     ASSERT_GE(stride, previous[quality]);
     previous[quality] = strides[quality] = stride;
    }
    ASSERT_LE(strides[0], strides[1]);
    ASSERT_LE(strides[1], strides[2]);
   }
  }
 }
}
}
