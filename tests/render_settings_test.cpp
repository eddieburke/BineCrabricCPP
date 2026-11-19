#include <gtest/gtest.h>
#include <limits>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
namespace option = net::minecraft::client::option;
TEST(RenderSettings, ChunkUpdatesUseStableFrameBudgets) {
 option::RenderSettings resolved;
 resolved.chunkUpdatesSlider = 0.0f;
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved), 1);
 resolved.chunkUpdatesSlider = 0.5f;
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved), 9);
 resolved.chunkUpdatesSlider = 1.0f;
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved), 16);
}
TEST(RenderSettings, DynamicChunkUpdatesScaleGraduallyWithBacklog) {
 option::RenderSettings resolved;
 resolved.chunkUpdatesSlider = 0.0f;
 resolved.chunkUpdatesDynamic = true;
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved, 2), 1);
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved, 3), 2);
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved, 11), 3);
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved, 1000), 16);
}
TEST(RenderSettings, DistantRangeDoesNotExpandFullChunkResidency) {
 option::GameOptions options;
 options.viewDistance = 0;
 options.renderScale = 5.0f;
 const option::RenderSettings resolved = option::renderSettings(options);
 EXPECT_FLOAT_EQ(resolved.renderScale, 5.0f);
 EXPECT_FLOAT_EQ(resolved.renderDistance.blocks, 1280.0f);
 EXPECT_EQ(resolved.renderDistance.chunks(), 80);
 EXPECT_EQ(resolved.nativeChunkRadius, 16);
 EXPECT_EQ(resolved.residentChunkRadius, 24);
}
TEST(RenderSettings, LongDistantRangeKeepsNativeResidencyBounded) {
 option::GameOptions options;
 auto resolved = option::renderSettings(options);
 EXPECT_TRUE(resolved.distantTerrain);
 EXPECT_EQ(resolved.renderDistance.chunks(), 128);
 options.renderScale = 32.0f;
 resolved = option::renderSettings(options);
 EXPECT_FLOAT_EQ(resolved.renderDistance.blocks, 8192.0f);
 EXPECT_EQ(resolved.renderDistance.chunks(), 512);
 EXPECT_EQ(resolved.residentChunkRadius, 24);
 options.preloadedChunks = std::numeric_limits<int>::max();
 resolved = option::renderSettings(options);
 EXPECT_EQ(resolved.residentChunkRadius, 32);
}
TEST(RenderSettings, UnsupportedShadersBoundNativeTerrainAndResidencyTogether) {
 option::GameOptions options;
 options.renderScale = 32.0f;
 net::minecraft::client::render::PackDefinition pack;
 pack.programs["gbuffers_terrain"] = {};
 const auto resolved = option::renderSettings(options, pack);
 EXPECT_FALSE(resolved.distantTerrain);
 EXPECT_EQ(resolved.nativeChunkRadius, 80);
 EXPECT_EQ(resolved.renderDistance.chunks(), 80);
 EXPECT_FLOAT_EQ(resolved.renderDistance.fogEnd(), 1280.0f);
 EXPECT_EQ(resolved.residentChunkRadius, 83);
 options.distantTerrain = false;
 const auto disabled = option::renderSettings(options);
 EXPECT_EQ(disabled.renderDistance, resolved.renderDistance);
 EXPECT_EQ(disabled.residentChunkRadius, resolved.residentChunkRadius);
}
TEST(RenderSettings, DistanceSliderReachesEveryIntegerScaleAndPersistsItsValue) {
 option::GameOptions options;
 const auto spec = option::OptionRegistry::byKey("renderScale");
 ASSERT_TRUE(spec.has_value());
 for(int scale = 1; scale <= 32; ++scale) {
  (*spec)->setFloat(options, static_cast<float>(scale - 1) / 31.0f);
  EXPECT_FLOAT_EQ(options.renderScale, static_cast<float>(scale));
  EXPECT_NEAR((*spec)->getFloat(options), static_cast<float>(scale - 1) / 31.0f, 1.0e-6f);
 }
 options.renderScale = std::numeric_limits<float>::infinity();
 EXPECT_FLOAT_EQ(option::renderSettings(options).renderScale, 1.0f);
 options.renderScale = -100.0f;
 EXPECT_FLOAT_EQ(option::renderSettings(options).renderScale, 1.0f);
 options.renderScale = 100.0f;
 EXPECT_FLOAT_EQ(option::renderSettings(options).renderScale, 32.0f);
}
