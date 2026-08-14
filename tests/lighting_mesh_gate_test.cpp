#include <gtest/gtest.h>
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/client/render/world/TerrainScene.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#define private public
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#undef private
namespace net::minecraft::test {
namespace {
struct LightingMeshFixture {
 ClientWorld world{nullptr, 12345ULL, 0};
 client::render::TerrainScene scene{};
 client::render::ChunkCompilePipeline pipeline;
 client::render::ChunkSectionSystem sections;
 LightingMeshFixture() : pipeline(scene), sections(scene) {
  scene.world = &world;
  sections.setCompilePipeline(pipeline);
  pipeline.setSectionSystem(sections);
 }
 void load(int x, int z) {
  world.updateChunk(x, z, true);
  world.getChunkSource()->markChunkDataReady(x, z);
 }
};
}
TEST(LightingMeshGate, FirstMeshCaptureWaitsForLightingReadiness) {
 LightingMeshFixture fixture;
 fixture.load(0, 0);
 fixture.sections.chunkAvailable(0, 0);
 fixture.sections.drainPendingColumns();
 for(int sectionY = 0; sectionY < client::render::kChunkSectionCountY; ++sectionY) {
  const auto* section = fixture.sections.sectionAt(0, sectionY, 0);
  ASSERT_NE(section, nullptr);
  EXPECT_TRUE(section->dirty);
  EXPECT_FALSE(section->lightingReady);
  EXPECT_FALSE(section->readyForMeshCapture());
 }
 fixture.sections.markChunkColumnLit(0, 0);
 for(int sectionY = 0; sectionY < client::render::kChunkSectionCountY; ++sectionY) {
  const auto* section = fixture.sections.sectionAt(0, sectionY, 0);
  ASSERT_NE(section, nullptr);
  EXPECT_TRUE(section->lightingReady);
  EXPECT_TRUE(section->readyForMeshCapture());
 }
}
TEST(LightingMeshGate, ReadinessBeforeSectionCreationIsPreserved) {
 LightingMeshFixture fixture;
 fixture.load(0, 0);
 fixture.sections.chunkAvailable(0, 0);
 fixture.sections.markChunkColumnLit(0, 0);
 fixture.sections.drainPendingColumns();
 const auto* section = fixture.sections.sectionAt(0, 0, 0);
 ASSERT_NE(section, nullptr);
 EXPECT_TRUE(section->lightingReady);
 EXPECT_TRUE(section->readyForMeshCapture());
}
TEST(LightingMeshGate, BorderRefreshWaitsForArrivingColumnLighting) {
 LightingMeshFixture fixture;
 fixture.load(0, 0);
 fixture.sections.createColumn(0, 0);
 auto* neighbor = fixture.sections.sectionAt(0, 0, 0);
 ASSERT_NE(neighbor, nullptr);
 neighbor->built = true;
 neighbor->dirty = false;
 fixture.load(1, 0);
 fixture.sections.chunkAvailable(1, 0);
 fixture.sections.drainPendingColumns();
 fixture.sections.drainBorderRefresh();
 EXPECT_FALSE(neighbor->dirty);
 EXPECT_TRUE(fixture.sections.pendingBorderRefresh_.contains(client::render::world::SectionPos{1, 0, 0}));
 fixture.sections.markChunkColumnLit(1, 0);
 fixture.sections.drainBorderRefresh();
 EXPECT_TRUE(neighbor->dirty);
 EXPECT_FALSE(fixture.sections.pendingBorderRefresh_.contains(client::render::world::SectionPos{1, 0, 0}));
}
}
