#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <future>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/world/ColumnRange.hpp"
#include "net/minecraft/client/render/world/TerrainScene.hpp"
#include "net/minecraft/util/concurrent/FrameBudget.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#define private public
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#include "net/minecraft/client/render/distant/DistantRenderer.hpp"
#undef private
namespace net::minecraft::test {
using namespace client::render;
using namespace util::concurrent;
TEST(WalkingChunkScheduling, EnteringDiscMatchesFullScanAcrossMovementAndRadiusChanges) {
 for(int oldRadius : {-1, 0, 4, 8}) {
  for(int radius : {0, 4, 8}) {
   for(int dx : {-20, -1, 0, 1, 20}) {
    for(int dz : {-20, -1, 0, 1, 20}) {
     std::set<std::pair<int, int>> actual;
     client::render::world::forEachEnteringColumn(-5 + dx, -7 + dz, radius, -5, -7, oldRadius,
                                   [&](int x, int z) { EXPECT_TRUE(actual.emplace(x, z).second); });
     std::set<std::pair<int, int>> expected;
     for(int z = -7 + dz - radius; z <= -7 + dz + radius; ++z) {
      for(int x = -5 + dx - radius; x <= -5 + dx + radius; ++x) {
       if(client::render::world::columnInRange(x, z, -5 + dx, -7 + dz, radius) &&
          !client::render::world::columnInRange(x, z, -5, -7, oldRadius)) expected.emplace(x, z);
      }
     }
     EXPECT_EQ(actual, expected);
    }
   }
  }
 }
 int visited = 0;
 client::render::world::forEachEnteringColumn(1, 0, 80, 0, 0, 80, [&](int, int) { ++visited; });
 EXPECT_EQ(visited, 161);
}
TEST(WalkingChunkScheduling, ExpiredSharedBudgetOnlyAllowsExplicitMinimum) {
 FrameBudget::beginFrame(0);
 const auto stopped = FrameBudget::fromSharedMs(1000, 0);
 EXPECT_FALSE(stopped.hasRemaining(0));
 const auto progress = FrameBudget::fromSharedMs(1000, 1);
 EXPECT_TRUE(progress.hasRemaining(0));
 EXPECT_FALSE(progress.hasRemaining(1));
 FrameBudget::beginFrame(16);
}
TEST(WalkingChunkScheduling, WorkerLimitChangesWithoutDroppingQueuedTasks) {
 WorkerPool pool(4);
 pool.setConcurrencyLimit(1);
 std::promise<void> release;
 auto released = release.get_future().share();
 std::promise<void> first;
 auto firstStarted = first.get_future();
 std::promise<void> third;
 auto thirdStarted = third.get_future();
 std::atomic<int> entered = 0;
 std::atomic<int> completed = 0;
 for(int i = 0; i < 12; ++i) {
  pool.submit([&] {
   const int count = ++entered;
   if(count == 1) first.set_value();
   if(count == 3) third.set_value();
   released.wait();
   ++completed;
  });
 }
 EXPECT_EQ(firstStarted.wait_for(std::chrono::seconds(2)), std::future_status::ready);
 EXPECT_EQ(entered.load(), 1);
 EXPECT_EQ(pool.concurrencyLimit(), 1u);
 pool.setConcurrencyLimit(3);
 EXPECT_EQ(thirdStarted.wait_for(std::chrono::seconds(2)), std::future_status::ready);
 EXPECT_EQ(entered.load(), 3);
 release.set_value();
 pool.drain();
 EXPECT_EQ(completed.load(), 12);
 pool.setConcurrencyLimit(100);
 EXPECT_EQ(pool.concurrencyLimit(), 4u);
 pool.setConcurrencyLimit(0);
 EXPECT_EQ(pool.concurrencyLimit(), 1u);
}
TEST(WalkingChunkScheduling, DirtyScanIsBoundedFairAndSafeAfterRemoval) {
 TerrainScene scene;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 pipeline.setSectionSystem(sections);
 std::vector<net::minecraft::block::entity::BlockEntity*> entities;
 std::vector<std::unique_ptr<chunk::ChunkBuilder>> builders;
 for(int i = 0; i < 1200; ++i) {
  auto builder = std::make_unique<chunk::ChunkBuilder>(nullptr, entities, i * 16, 0, 0);
  builder->dirty = true;
  pipeline.enqueueDirtyChunk(builder.get());
  pipeline.enqueueDirtyChunk(builder.get());
  builders.push_back(std::move(builder));
 }
 EXPECT_EQ(pipeline.dirtyChunks_.size(), 1200u);
 std::set<chunk::ChunkBuilder*> seen;
 for(int pass = 0; pass < 3; ++pass) {
  pipeline.gatherCaptureCandidates(false);
  EXPECT_EQ(pipeline.captureCandidates_.size(), 512u);
  seen.insert(pipeline.captureCandidates_.begin(), pipeline.captureCandidates_.end());
 }
 EXPECT_EQ(seen.size(), 1200u);
 pipeline.releaseSection(*builders[0]);
 builders[0].reset();
 pipeline.gatherCaptureCandidates(false);
 EXPECT_EQ(pipeline.dirtyChunks_.size(), 1199u);
 pipeline.clearDirtyTracking();
 EXPECT_TRUE(pipeline.dirtyScan_.empty());
}
TEST(WalkingChunkScheduling, PendingColumnHeapTracksCameraAndNewArrivals) {
 TerrainScene scene;
 ChunkSectionSystem sections(scene);
 sections.meshCameraX_ = -10;
 sections.meshCameraZ_ = -10;
 for(const auto [x, z] : {std::pair{-2, -2}, {-9, -10}, {-10, -10}, {-7, -8}}) sections.enqueueColumn(x, z);
 EXPECT_EQ(sections.pendingColumns_.front(), (client::render::world::ColumnPos{-10, -10}));
 sections.meshCameraX_ = -2;
 sections.meshCameraZ_ = -2;
 const auto after = [&](const auto& a, const auto& b) { return sections.pendingColumnAfter(a, b); };
 std::make_heap(sections.pendingColumns_.begin(), sections.pendingColumns_.end(), after);
 EXPECT_EQ(sections.pendingColumns_.front(), (client::render::world::ColumnPos{-2, -2}));
 sections.enqueueColumn(-2, -3);
 EXPECT_TRUE(std::is_heap(sections.pendingColumns_.begin(), sections.pendingColumns_.end(), after));
}
TEST(WalkingChunkScheduling, RetirementRechecksCameraRangeBeforeFreeingSections) {
 ClientWorld worldInstance(nullptr, 12345ULL, 0);
 worldInstance.updateChunk(1, 0, true);
 worldInstance.getChunkSource()->markChunkDataReady(1, 0);
 TerrainScene scene;
 scene.world = &worldInstance;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 pipeline.setSectionSystem(sections);
 sections.setCompilePipeline(pipeline);
 sections.createColumn(1, 0);
 ASSERT_NE(sections.sectionAt(1, 0, 0), nullptr);
 client::option::RenderSettings settings;
 scene.settings = &settings;
 sections.meshCameraX_ = 0;
 sections.meshCameraZ_ = 0;
 sections.meshRadius_ = 3;
 sections.retiredColumns_.push_back({1, 0});
 sections.retiredSet_.insert({1, 0});
 sections.drainRetiredColumns();
 EXPECT_NE(sections.sectionAt(1, 0, 0), nullptr);
 EXPECT_TRUE(sections.retiredSet_.empty());
 sections.meshCameraX_ = -10;
 sections.retiredColumns_.push_back({1, 0});
 sections.retiredSet_.insert({1, 0});
 sections.drainRetiredColumns();
 EXPECT_EQ(sections.sectionAt(1, 0, 0), nullptr);
 sections.clearSections();
}
TEST(WalkingChunkScheduling, DistantGridIsCachedAndInvalidatesOnSettings) {
 distant::DistantRenderer renderer;
 client::option::RenderSettings settings;
 settings.renderDistance.blocks = 1280;
 renderer.nativeRadius_ = settings.nativeChunkRadius;
 ++renderer.frame_;
 EXPECT_TRUE(renderer.updateWanted(settings, -15.99, -15.99));
 const auto size = renderer.wanted_.size();
 for(int i = 0; i < 300; ++i) {
  ++renderer.frame_;
  EXPECT_FALSE(renderer.updateWanted(settings, -15.99 + i * 0.05, -15.99));
 }
 EXPECT_EQ(renderer.wanted_.size(), size);
 ++renderer.frame_;
 EXPECT_TRUE(renderer.updateWanted(settings, 0.01, -15.99));
 ++renderer.frame_;
 settings.lodQuality = 2;
 EXPECT_TRUE(renderer.updateWanted(settings, 0.01, -15.99));
 ++renderer.frame_;
 renderer.memoryStride_ = 2;
 EXPECT_TRUE(renderer.updateWanted(settings, 0.01, -15.99));
}
TEST(WalkingChunkScheduling, CachedDistantGridCoversEveryCornerOfCameraCell) {
 distant::DistantRenderer renderer;
 client::option::RenderSettings settings;
 settings.renderDistance.blocks = 1024;
 renderer.nativeRadius_ = settings.nativeChunkRadius;
 ++renderer.frame_;
 renderer.updateWanted(settings, -0.01, -0.01);
 const double inner = renderer.nativeRadius_ * 16 - distant::kNativeOverlapBlocks;
 for(double eyeX : {-15.999, -0.001}) {
  for(double eyeZ : {-15.999, -0.001}) {
   for(int tz = -18; tz <= 18; ++tz) {
    for(int tx = -18; tx <= 18; ++tx) {
     const double minX = tx * distant::kTileBlocks;
     const double minZ = tz * distant::kTileBlocks;
     const double dx = std::clamp(eyeX, minX, minX + distant::kTileBlocks) - eyeX;
     const double dz = std::clamp(eyeZ, minZ, minZ + distant::kTileBlocks) - eyeZ;
     const double farX = std::max(std::abs(minX - eyeX), std::abs(minX + distant::kTileBlocks - eyeX));
     const double farZ = std::max(std::abs(minZ - eyeZ), std::abs(minZ + distant::kTileBlocks - eyeZ));
     if(dx * dx + dz * dz <= settings.renderDistance.blocks * settings.renderDistance.blocks &&
        farX * farX + farZ * farZ >= inner * inner) {
      EXPECT_TRUE(renderer.tiles_.contains({tx, tz})) << tx << "," << tz;
     }
    }
   }
  }
 }
 renderer.collectBuildCandidates(2);
 EXPECT_LE(renderer.buildCandidates_.size(), 2u);
}
TEST(WalkingChunkScheduling, BackgroundThreadSettingIsRegisteredAndPersisted) {
 auto spec = client::option::OptionRegistry::byKey("backgroundThreads");
 ASSERT_TRUE(spec.has_value());
 client::option::GameOptions options;
 (*spec)->load(options, "1");
 EXPECT_EQ(options.backgroundThreads, 1);
 std::ostringstream saved;
 (*spec)->save(options, saved);
 EXPECT_EQ(saved.str(), "1");
 options.backgroundThreads = 0;
 (*spec)->load(options, saved.str());
 EXPECT_EQ(options.backgroundThreads, 1);
}
TEST(WalkingChunkScheduling, ReportsWalkingGridRebuildWork) {
 client::option::RenderSettings settings;
 settings.renderDistance.blocks = 1280;
 const auto measure = [&](bool cache) {
  distant::DistantRenderer renderer;
  renderer.nativeRadius_ = settings.nativeChunkRadius;
  int rebuilds = 0;
  const auto start = std::chrono::steady_clock::now();
  for(int frame = 0; frame < 900; ++frame) {
   ++renderer.frame_;
   if(!cache) renderer.wantedValid_ = false;
   rebuilds += renderer.updateWanted(settings, frame * 0.075, 0.0);
  }
  const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
  return std::pair{rebuilds, micros};
 };
 const auto uncached = measure(false);
 const auto cached = measure(true);
 EXPECT_EQ(uncached.first, 900);
 EXPECT_EQ(cached.first, 5);
 std::printf("Walking grid, 900 frames at 1280 blocks: uncached %d rebuilds / %lld us; cached %d rebuilds / %lld us\n",
              uncached.first, static_cast<long long>(uncached.second), cached.first, static_cast<long long>(cached.second));
}

}
