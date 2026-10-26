#include <gtest/gtest.h>
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/client/render/world/TerrainScene.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/events/GameEventListener.hpp"
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
  Chunk& chunk = world.getChunk(x, z);
  for(int sectionY = 0; sectionY < client::render::kChunkSectionCountY; ++sectionY) {
   chunk.blocks[static_cast<std::size_t>(sectionY * 16)] = 1;
  }
  chunk.populateHeightMapOnly();
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
 EXPECT_TRUE(fixture.sections.pendingBorderRefresh_.contains(client::render::world::ColumnPos{1, 0}));
 fixture.sections.markChunkColumnLit(1, 0);
 fixture.sections.drainBorderRefresh();
 EXPECT_TRUE(neighbor->dirty);
 EXPECT_FALSE(fixture.sections.pendingBorderRefresh_.contains(client::render::world::ColumnPos{1, 0}));
}
TEST(LightingMeshGate, WorldPublishesCompletionEvenWhenNoLightChanged) {
 LightingMeshFixture fixture;
 fixture.load(0, 0);
 fixture.sections.chunkAvailable(0, 0);
 fixture.sections.drainPendingColumns();
 struct Listener final : GameEventListener {
  client::render::ChunkSectionSystem& sections;
  int completions = 0;
  explicit Listener(client::render::ChunkSectionSystem& value) : sections(value) {}
  void markChunkColumnLit(int x, int z) override {
   ++completions;
   sections.markChunkColumnLit(x, z);
  }
 } listener(fixture.sections);
 fixture.world.addEventListener(&listener);
 EXPECT_FALSE(fixture.sections.columnLightingReady(0, 0));
 fixture.world.doLightingUpdates(1);
 EXPECT_EQ(listener.completions, 1);
 EXPECT_TRUE(fixture.sections.columnLightingReady(0, 0));
 EXPECT_TRUE(fixture.sections.sectionAt(0, 0, 0)->readyForMeshCapture());
 fixture.world.removeEventListener(&listener);
}
TEST(LightingMeshGate, GlobalCompletionDoesNotBypassBoundedPerColumnNotifications) {
 LightingMeshFixture fixture;
 for(int x = 0; x < 3; ++x) {
  fixture.load(x, 0);
  fixture.sections.chunkAvailable(x, 0);
 }
 struct Listener final : GameEventListener {
  client::render::ChunkSectionSystem& sections;
  int columns = 0;
  int global = 0;
  explicit Listener(client::render::ChunkSectionSystem& value) : sections(value) {}
  void markChunkColumnLit(int x, int z) override {
   ++columns;
   sections.markChunkColumnLit(x, z);
  }
  void markAllChunksLit() override {
   ++global;
   sections.markAllChunksLit();
  }
 } listener(fixture.sections);
 fixture.world.addEventListener(&listener);
 EXPECT_TRUE(fixture.world.doLightingUpdates(1));
 EXPECT_EQ(listener.columns, 1);
 EXPECT_EQ(listener.global, 0);
 EXPECT_TRUE(fixture.world.doLightingUpdates(1));
 EXPECT_EQ(listener.columns, 2);
 EXPECT_EQ(listener.global, 0);
 EXPECT_FALSE(fixture.world.doLightingUpdates(1));
 EXPECT_EQ(listener.columns, 3);
 EXPECT_EQ(listener.global, 1);
 fixture.world.removeEventListener(&listener);
}
}
