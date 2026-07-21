#include <gtest/gtest.h>
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
namespace option = net::minecraft::client::option;
TEST(ResolvedRenderOptions, ChunkUpdatesUseStableFrameBudgets) {
 option::ResolvedRenderOptions resolved;
 resolved.chunkUpdatesSlider = 0.0f;
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved), 1);
 resolved.chunkUpdatesSlider = 0.5f;
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved), 9);
 resolved.chunkUpdatesSlider = 1.0f;
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved), 16);
}
TEST(ResolvedRenderOptions, DynamicChunkUpdatesScaleGraduallyWithBacklog) {
 option::ResolvedRenderOptions resolved;
 resolved.chunkUpdatesSlider = 0.0f;
 resolved.chunkUpdatesDynamic = true;
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved, 2), 1);
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved, 3), 2);
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved, 11), 3);
 EXPECT_EQ(option::chunkUpdatesPerPass(resolved, 1000), 16);
}
TEST(ResolvedRenderOptions, LodKeepsFullDetailResidencyBounded) {
 option::GameOptions options;
 options.viewDistance = 0;
 options.renderScale = 5.0f;
 options.lodEnabled = true;
 const option::ResolvedRenderOptions resolved = option::resolve(options);
 EXPECT_FLOAT_EQ(resolved.renderDistanceBlocks, 1280.0f);
 EXPECT_EQ(resolved.chunkRadius, 13);
 EXPECT_EQ(resolved.residentChunkRadius, 16);
}
TEST(ResolvedRenderOptions, RenderScaleExpandsFullDetailWithoutLod) {
 option::GameOptions options;
 options.viewDistance = 0;
 options.renderScale = 5.0f;
 options.lodEnabled = false;
 const option::ResolvedRenderOptions resolved = option::resolve(options);
 EXPECT_EQ(resolved.chunkRadius, 63);
 EXPECT_EQ(resolved.residentChunkRadius, 66);
}
