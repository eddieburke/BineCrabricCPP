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
#include "net/minecraft/client/render/world/TerrainScene.hpp"
#include "net/minecraft/util/concurrent/FrameBudget.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#define private public
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#include "net/minecraft/client/render/distant/DistantRenderer.hpp"
#undef private
namespace net::minecraft::test {
using namespace client::render;
using namespace util::concurrent;
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
TEST(WalkingChunkScheduling, WaitingForLightingDoesNotBlockReadyNearSections) {
 ClientWorld worldInstance(nullptr, 12345ULL, 0);
 worldInstance.updateChunk(0, 0, true);
 worldInstance.updateChunk(1, 0, true);
 TerrainScene scene;
 client::option::RenderSettings settings;
 entity::LivingEntity camera(&worldInstance);
 camera.setPosition(1000.0, 65.0, 1000.0);
 scene.world = &worldInstance;
 scene.settings = &settings;
 scene.camera = &camera;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 pipeline.setSectionSystem(sections);
 sections.setCompilePipeline(pipeline);
 auto a = std::make_shared<chunk::ChunkBuilder>(&worldInstance, scene.blockEntities, 0, 64, 0);
 auto b = std::make_shared<chunk::ChunkBuilder>(&worldInstance, scene.blockEntities, 16, 64, 0);
 a->invalidate();
 b->invalidate();
 pipeline.enqueueDirtyChunk(a.get());
 pipeline.enqueueDirtyChunk(b.get());
 pipeline.nearDirtyChunks_.insert(a.get());
 pipeline.nearDirtyChunks_.insert(b.get());
 chunk::ChunkBuilder* blocked = *pipeline.nearDirtyChunks_.begin();
 chunk::ChunkBuilder* ready = blocked == a.get() ? b.get() : a.get();
 blocked->lightingReady = false;
 FrameBudget::beginFrame(0);
 pipeline.compileChunks(camera, false);
 EXPECT_TRUE(ready->meshJobInFlight);
 EXPECT_FALSE(blocked->meshJobInFlight);
 pipeline.cancelAll();
 pipeline.clearDirtyTracking();
 FrameBudget::beginFrame(16);
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

}
