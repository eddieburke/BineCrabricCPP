#include <gtest/gtest.h>
#include <limits>
#include "net/minecraft/client/render/chunk/MeshUploadBudget.hpp"
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#define private public
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#undef private
namespace net::minecraft::test {
using namespace client::render;
TEST(TerrainCoverage, DiscHandlesSignedCoordinatesAndOverflow) {
 EXPECT_TRUE(client::render::world::columnInRange(-10, -20, -10, -20, 0));
 EXPECT_TRUE(client::render::world::columnInRange(-7, -16, -10, -20, 5));
 EXPECT_FALSE(client::render::world::columnInRange(-6, -16, -10, -20, 5));
 EXPECT_FALSE(client::render::world::columnInRange(0, 0, 0, 0, -1));
 EXPECT_FALSE(client::render::world::columnInRange(std::numeric_limits<int>::min(), 0, std::numeric_limits<int>::max(), 0, 80));
}
TEST(TerrainCoverage, LightingGateSharesNativeCompletionState) {
 ClientWorld world{nullptr, 12345ULL, 0};
 world.updateChunk(0, 0, true);
 world.getChunkSource()->markChunkDataReady(0, 0);
 TerrainScene scene;
 scene.world = &world;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 sections.setCompilePipeline(pipeline);
 pipeline.setSectionSystem(sections);
 sections.chunkAvailable(0, 0);
 EXPECT_FALSE(sections.columnLightingReady(0, 0));
 sections.markChunkColumnLit(0, 0);
 EXPECT_TRUE(sections.columnLightingReady(0, 0));
 sections.chunkAvailable(0, 0);
 EXPECT_FALSE(sections.columnLightingReady(0, 0));
 sections.markAllChunksLit();
 EXPECT_TRUE(sections.columnLightingReady(0, 0));
 sections.clearSections();
}
TEST(TerrainUploadBudget, BoundsCombinedLanesAndAllowsOneOversizeMeshToProgress) {
 chunk::MeshUploadBudget budget(100);
 EXPECT_TRUE(budget.tryConsume(60));
 EXPECT_FALSE(budget.tryConsume(50));
 EXPECT_TRUE(budget.tryConsume(40));
 EXPECT_FALSE(budget.tryConsume(1));
 EXPECT_EQ(budget.used(), 100u);
 chunk::MeshUploadBudget oversize(100);
 EXPECT_TRUE(oversize.tryConsume(200));
 EXPECT_FALSE(oversize.tryConsume(1));
}
}
