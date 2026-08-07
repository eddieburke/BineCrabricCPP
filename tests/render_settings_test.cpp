#include <gtest/gtest.h>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
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
TEST(RenderSettings, RenderScaleExpandsTerrainResidency) {
 option::GameOptions options;
 options.viewDistance = 0;
 options.renderScale = 5.0f;
 const option::RenderSettings resolved = option::renderSettings(options);
 EXPECT_FLOAT_EQ(resolved.renderDistance.blocks, 1280.0f);
 EXPECT_EQ(resolved.renderDistance.chunks(), 80);
 EXPECT_EQ(resolved.residentChunkRadius, 83);
}
