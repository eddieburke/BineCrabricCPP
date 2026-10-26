#include <gtest/gtest.h>
#include <cmath>
#include <future>
#include <limits>
#include <sstream>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#define private public
#include "net/minecraft/client/render/distant/DistantRenderer.hpp"
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#undef private
namespace net::minecraft::test {
namespace {
using namespace client::render;
}
TEST(LodTransition, NativeBoundarySurvivesUntilItsDistantColumnIsUploaded) {
 ClientWorld world{nullptr, 12345ULL, 0};
 world.updateChunk(18, 0, true);
 world.getChunkSource()->markChunkDataReady(18, 0);
 client::option::GameOptions options;
 options.renderScale = 2.0f;
 auto settings = client::option::renderSettings(options);
 entity::LivingEntity camera(&world);
 distant::DistantRenderer renderer;
 TerrainScene scene;
 scene.world = &world;
 scene.camera = &camera;
 scene.settings = &settings;
 scene.options = &options;
 scene.distant = &renderer;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 sections.setCompilePipeline(pipeline);
 pipeline.setSectionSystem(sections);
 sections.reloadIfViewDistanceChanged();
 sections.createColumn(18, 0);
 ASSERT_NE(sections.sectionAt(18, 0, 0), nullptr);
 const distant::DistantRenderer::TileKey key{4, 0};
 auto& tile = renderer.tiles_[key];
 tile.inFlight = true;
 tile.jobId = 1;
 renderer.inFlight_ = 1;
 distant::DistantRenderer::Completed result;
 result.key = key;
 result.jobId = 1;
 result.chunkMask = 1u << 2;
 renderer.shared_->completed.push_back(std::move(result));
 EXPECT_FALSE(renderer.coversColumn(18, 0));
 sections.reloadIfViewDistanceChanged();
 EXPECT_NE(sections.sectionAt(18, 0, 0), nullptr);
 renderer.drainCompleted(1024);
 EXPECT_TRUE(renderer.coversColumn(18, 0));
 EXPECT_FALSE(renderer.coversColumn(19, 0));
 sections.reloadIfViewDistanceChanged();
 EXPECT_EQ(sections.sectionAt(18, 0, 0), nullptr);
 sections.clearSections();
}
TEST(LodTransition, OldTileCompletionDoesNotClearReplacementJob) {
 distant::DistantRenderer renderer;
 const distant::DistantRenderer::TileKey key{-1, -1};
 auto& tile = renderer.tiles_[key];
 tile.inFlight = true;
 tile.jobId = 2;
 renderer.inFlight_ = 2;
 distant::DistantRenderer::Completed result;
 result.key = key;
 result.jobId = 1;
 result.chunkMask = 0xFFFFu;
 renderer.shared_->completed.push_back(std::move(result));
 renderer.drainCompleted(1024);
 EXPECT_TRUE(tile.inFlight);
 EXPECT_EQ(renderer.inFlight_, 1);
 EXPECT_FALSE(renderer.coversColumn(-1, -1));
 EXPECT_EQ(renderer.coverageRevision, 0u);
}
TEST(LodTransition, FirstCoverageCanPublishDuringArrivalsButUnloadedSourcesAreRejected) {
 for(const bool unloaded : {false, true}) {
  distant::DistantRenderer renderer;
  auto& tile = renderer.tiles_[{4, 0}];
  tile.inFlight = true;
  tile.jobId = 1;
  renderer.inFlight_ = 1;
  renderer.markChunkDirty(16, 0, unloaded);
  distant::DistantRenderer::Completed result;
  result.key = {4, 0};
  result.jobId = 1;
  result.chunkMask = 1;
  renderer.shared_->completed.push_back(std::move(result));
  renderer.drainCompleted(1024);
  EXPECT_EQ(renderer.coversColumn(16, 0), !unloaded);
  EXPECT_LT(tile.builtGeneration, tile.generation);
 }
}
TEST(LodTransition, StaleRefreshCannotReplaceExistingCoverage) {
 distant::DistantRenderer renderer;
 auto& tile = renderer.tiles_[{4, 0}];
 tile.stride = 1;
 tile.chunkMask = 0xFFFFu;
 tile.generation = 2;
 tile.inFlight = true;
 tile.jobId = 1;
 renderer.inFlight_ = 1;
 distant::DistantRenderer::Completed result;
 result.key = {4, 0};
 result.jobId = 1;
 result.generation = 1;
 result.chunkMask = 1;
 renderer.shared_->completed.push_back(std::move(result));
 renderer.drainCompleted(1024);
 EXPECT_TRUE(renderer.coversColumn(19, 0));
 EXPECT_EQ(tile.chunkMask, 0xFFFFu);
}
TEST(LodTransition, FailedCompletionCannotEraseCoverageOrMarkTheTileClean) {
 distant::DistantRenderer renderer;
 auto& tile = renderer.tiles_[{4, 0}];
 tile.stride = 1;
 tile.chunkMask = 0xFFFFu;
 tile.generation = 1;
 tile.inFlight = true;
 tile.jobId = 1;
 renderer.inFlight_ = 1;
 distant::DistantRenderer::Completed failed;
 failed.key = {4, 0};
 failed.jobId = 1;
 failed.generation = 1;
 renderer.shared_->completed.push_back(std::move(failed));
 renderer.drainCompleted(1024);
 EXPECT_TRUE(renderer.coversColumn(16, 0));
 EXPECT_EQ(tile.builtGeneration, 0u);
 EXPECT_FALSE(tile.inFlight);
 EXPECT_EQ(renderer.inFlight_, 0);
 EXPECT_GT(tile.retryFrame, renderer.frame_);
}
TEST(LodTransition, ProgressiveCoverageSurvivesFurtherArrivalsWithoutDroppingExistingColumns) {
 distant::DistantRenderer renderer;
 auto& tile = renderer.tiles_[{4, 0}];
 tile.stride = 1;
 tile.chunkMask = 1;
 tile.generation = 2;
 tile.inFlight = true;
 tile.jobId = 1;
 renderer.inFlight_ = 1;
 distant::DistantRenderer::Completed result;
 result.key = {4, 0};
 result.jobId = 1;
 result.generation = 1;
 result.chunkMask = 3;
 renderer.shared_->completed.push_back(std::move(result));
 renderer.drainCompleted(1024);
 EXPECT_TRUE(renderer.coversColumn(16, 0));
 EXPECT_TRUE(renderer.coversColumn(17, 0));
 EXPECT_FALSE(renderer.coversColumn(18, 0));
 EXPECT_LT(tile.builtGeneration, tile.generation);
 EXPECT_EQ(renderer.coverageRevision, 1u);
}
TEST(LodTransition, RefreshWithUnchangedCoverageDoesNotReconcileNativeColumns) {
 distant::DistantRenderer renderer;
 auto& tile = renderer.tiles_[{4, 0}];
 tile.stride = 1;
 tile.chunkMask = 0xFFFFu;
 tile.generation = 1;
 tile.inFlight = true;
 tile.jobId = 1;
 renderer.inFlight_ = 1;
 distant::DistantRenderer::Completed result;
 result.key = {4, 0};
 result.jobId = 1;
 result.generation = 1;
 result.chunkMask = 0xFFFFu;
 renderer.shared_->completed.push_back(std::move(result));
 renderer.drainCompleted(1024);
 EXPECT_EQ(tile.builtGeneration, 1u);
 EXPECT_EQ(renderer.coverageRevision, 0u);
}
TEST(LodTransition, SubmissionSlotsFavorCoverageAndMemoryRecoveryOverNearbyRefreshes) {
 distant::DistantRenderer renderer;
 for(int x = 1; x <= 6; ++x) {
  auto& tile = renderer.tiles_[{x, 0}];
  tile.stride = tile.wantedStride = 1;
  tile.chunkMask = 0xFFFFu;
  tile.generation = 1;
  renderer.wanted_.emplace_back(x * 64, distant::DistantRenderer::TileKey{x, 0});
 }
 renderer.tiles_.at({2, 0}).inFlight = true;
 renderer.tiles_.at({3, 0}).retryFrame = 10;
 renderer.tiles_.at({4, 0}).stride = 0;
 renderer.tiles_.at({4, 0}).chunkMask = 0;
 renderer.tiles_.at({5, 0}).chunkMask = 1;
 renderer.tiles_.at({6, 0}).wantedStride = 2;
 renderer.collectBuildCandidates();
 ASSERT_EQ(renderer.buildCandidates_.size(), 4u);
 EXPECT_EQ(renderer.buildCandidates_[0].second.x, 6);
 EXPECT_EQ(renderer.buildCandidates_[1].second.x, 4);
 EXPECT_EQ(renderer.buildCandidates_[2].second.x, 5);
 EXPECT_EQ(renderer.buildCandidates_[3].second.x, 1);
}
TEST(LodTransition, ResetCancelsOnlyOwnedQueuedJobsAndReleasesTheirPins) {
 util::concurrent::WorkerPool pool(1);
 std::promise<void> entered;
 std::promise<void> release;
 auto released = release.get_future();
 pool.submit([&] { entered.set_value(); released.wait(); });
 entered.get_future().wait();
 struct Unblock {
  std::promise<void>& release;
  ~Unblock() { release.set_value(); }
 } unblock{release};
 Chunk source(nullptr, 0, 0);
 distant::DistantRenderer renderer;
 renderer.pool_ = &pool;
 renderer.poolLifetime_ = pool.lifetimeToken();
 {
  auto pins = std::make_shared<distant::DistantRenderer::PinnedChunks>();
  ASSERT_TRUE(source.tryAcquireRenderPin());
  pins->chunks[0] = &source;
  pool.submit([pins] {}, 1, &renderer);
 }
 pool.submit([] {}, 1, &source);
 EXPECT_EQ(source.renderPinCount(), 1u);
 renderer.reset();
 EXPECT_EQ(source.renderPinCount(), 0u);
 EXPECT_EQ(pool.pendingCount(), 2u);
}
TEST(LodTransition, LargeRadiusShrinkRetainsNativeTerrainBeyondTheTemporaryRingUntilUpload) {
 ClientWorld world{nullptr, 12345ULL, 0};
 world.updateChunk(20, 0, true);
 world.getChunkSource()->markChunkDataReady(20, 0);
 client::option::GameOptions options;
 options.renderScale = 3;
 options.lodNativeRadius = 24;
 auto settings = client::option::renderSettings(options);
 entity::LivingEntity camera(&world);
 distant::DistantRenderer renderer;
 TerrainScene scene;
 scene.world = &world;
 scene.camera = &camera;
 scene.options = &options;
 scene.settings = &settings;
 scene.distant = &renderer;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 sections.setCompilePipeline(pipeline);
 pipeline.setSectionSystem(sections);
 sections.reloadIfViewDistanceChanged();
 sections.createColumn(20, 0);
 auto* original = sections.sectionAt(20, 0, 0);
 ASSERT_NE(original, nullptr);
 options.lodNativeRadius = 8;
 settings = client::option::renderSettings(options);
 renderer.updateNativeRadius(sections, 8);
 sections.reloadIfViewDistanceChanged();
 EXPECT_EQ(sections.sectionAt(20, 0, 0), original);
 auto& tile = renderer.tiles_[{5, 0}];
 tile.jobId = 1;
 tile.inFlight = true;
 renderer.inFlight_ = 1;
 distant::DistantRenderer::Completed result;
 result.key = {5, 0};
 result.jobId = 1;
 result.chunkMask = 1;
 renderer.shared_->completed.push_back(std::move(result));
 renderer.drainCompleted(1024);
 sections.reloadIfViewDistanceChanged();
 EXPECT_EQ(sections.sectionAt(20, 0, 0), nullptr);
 sections.clearSections();
}
TEST(LodTransition, GrowingNativeRadiusCommitsOnlyAfterEveryReplacementSectionIsBuilt) {
 TerrainScene scene;
 ChunkSectionSystem sections(scene);
 ChunkCompilePipeline pipeline(scene);
 sections.setCompilePipeline(pipeline);
 pipeline.setSectionSystem(sections);
 sections.meshRadius_ = 3;
 sections.meshCameraX_ = sections.meshCameraZ_ = 0;
 distant::DistantRenderer renderer;
 renderer.updateNativeRadius(sections, 1);
 renderer.updateNativeRadius(sections, 2);
 EXPECT_EQ(renderer.nativeRadius_, 1);
 for(int z = -2; z <= 2; ++z) {
  for(int x = -2; x <= 2; ++x) {
   if(x * x + z * z > 4) continue;
   auto& column = sections.columns_[{x, z}];
   for(int y = 0; y < kChunkSectionCountY; ++y) {
    column[y] = std::make_shared<chunk::ChunkBuilder>(nullptr, scene.blockEntities, x * 16, y * 16, z * 16);
    column[y]->built = true;
   }
  }
 }
 sections.columns_.at({2, 0})[0]->built = false;
 renderer.updateNativeRadius(sections, 2);
 EXPECT_EQ(renderer.nativeRadius_, 1);
 sections.columns_.at({2, 0})[0]->built = true;
 renderer.updateNativeRadius(sections, 2);
 EXPECT_EQ(renderer.nativeRadius_, 2);
 renderer.updateNativeRadius(sections, 1);
 EXPECT_EQ(renderer.nativeRadius_, 1);
 sections.clearSections();
}
TEST(LodTransition, FirstLodJobsPrecedeFarLoadingAndRefreshesWhileNativeWorkKeepsPriority) {
 util::concurrent::WorkerPool pool(1);
 std::promise<void> entered;
 std::promise<void> release;
 auto released = release.get_future();
 pool.submit([&] { entered.set_value(); released.wait(); }, std::numeric_limits<int>::min());
 entered.get_future().wait();
 std::vector<int> order;
 pool.submit([&] { order.push_back(3); }, 32 * 32);
 pool.submit([&] { order.push_back(2); }, distant::DistantRenderer::meshPriority(512, true));
 pool.submit([&] { order.push_back(1); }, distant::DistantRenderer::meshPriority(512, false));
 pool.submit([&] { order.push_back(0); }, -4);
 release.set_value();
 pool.drain();
 EXPECT_EQ(order, (std::vector<int>{0, 1, 2, 3}));
}
TEST(LodTransition, DisablingDistantProviderPreservesAndExpandsNativeCoverage) {
 ClientWorld world{nullptr, 12345ULL, 0};
 world.updateChunk(18, 0, true);
 world.getChunkSource()->markChunkDataReady(18, 0);
 world.updateChunk(40, 0, true);
 world.getChunkSource()->markChunkDataReady(40, 0);
 client::option::GameOptions options;
 options.renderScale = 3.0f;
 options.lodNativeRadius = 24;
 auto settings = client::option::renderSettings(options);
 ASSERT_TRUE(settings.distantTerrain);
 entity::LivingEntity camera(&world);
 TerrainScene scene;
 scene.world = &world;
 scene.camera = &camera;
 scene.settings = &settings;
 scene.options = &options;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 sections.setCompilePipeline(pipeline);
 pipeline.setSectionSystem(sections);
 sections.reloadIfViewDistanceChanged();
 sections.createColumn(18, 0);
 auto* original = sections.sectionAt(18, 0, 0);
 ASSERT_NE(original, nullptr);
 sections.createColumn(40, 0);
 EXPECT_EQ(sections.sectionAt(40, 0, 0), nullptr);
 options.distantTerrain = false;
 settings = client::option::renderSettings(options);
 sections.reloadIfViewDistanceChanged();
 EXPECT_EQ(sections.sectionAt(18, 0, 0), original);
 EXPECT_EQ(settings.nativeChunkRadius, 48);
 sections.createColumn(40, 0);
 EXPECT_NE(sections.sectionAt(40, 0, 0), nullptr);
 sections.clearSections();
}
TEST(LodTransition, OverlapStaysInsideNativeDiscAtEveryCameraSubchunkOffset) {
 constexpr double pi = 3.14159265358979323846;
 for(const int radius : {8, 12, 16, 24, 32}) {
  const double cutoff = radius * 16 - distant::kNativeOverlapBlocks;
  for(const int origin : {-100, 0, 100}) {
   for(const double offsetX : {0.0, 7.9, 15.99}) {
    for(const double offsetZ : {0.0, 7.9, 15.99}) {
     for(int angle = 0; angle < 360; ++angle) {
      const double radians = angle * pi / 180.0;
      const int x = static_cast<int>(std::floor((origin * 16 + offsetX + cutoff * std::cos(radians)) / 16));
      const int z = static_cast<int>(std::floor((origin * 16 + offsetZ + cutoff * std::sin(radians)) / 16));
      ASSERT_TRUE(client::render::world::columnInRange(x, z, origin, origin, radius + 1))
          << radius << " " << offsetX << " " << offsetZ << " " << angle;
     }
    }
   }
  }
 }
}
TEST(LodTransition, FullSlabCanShrinkAReplacementInPlaceAndRecoverFromPressure) {
 ASSERT_EQ(glfwInit(), GLFW_TRUE);
 glfwDefaultWindowHints();
 glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
 glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 struct WindowScope {
  GLFWwindow* window = glfwCreateWindow(32, 32, "lod-budget-test", nullptr, nullptr);
  ~WindowScope() {
   client::render::core::releaseGlResources();
   glfwMakeContextCurrent(nullptr);
   if(window != nullptr) glfwDestroyWindow(window);
  }
 } context;
 ASSERT_NE(context.window, nullptr);
 glfwMakeContextCurrent(context.window);
 client::gl::GLCore::ensureLoaded();
 distant::DistantRenderer renderer;
 constexpr std::size_t pageBytes = distant::DistantSlab::kPageVertices * sizeof(distant::DhVertex);
 ASSERT_TRUE(renderer.slab_.create(pageBytes * 4));
 const distant::DistantRenderer::TileKey key{4, 0};
 auto& tile = renderer.tiles_[key];
 tile.opaque = renderer.slab_.allocate(distant::DistantSlab::kPageVertices * 4);
 ASSERT_TRUE(tile.opaque.has_value());
 EXPECT_EQ(renderer.slab_.usedBytes(), pageBytes * 4);
 const distant::DistantRenderer::TileKey blockedKey{5, 0};
 auto& blocked = renderer.tiles_[blockedKey];
 blocked.jobId = 1;
 blocked.inFlight = true;
 renderer.inFlight_ = 1;
 distant::DistantRenderer::Completed failed;
 failed.key = blockedKey;
 failed.jobId = 1;
 failed.chunkMask = 0xFFFFu;
 failed.mesh.opaque.resize(4);
 renderer.shared_->completed.push_back(std::move(failed));
 const distant::DistantRenderer::TileKey secondBlockedKey{6, 0};
 auto& secondBlocked = renderer.tiles_[secondBlockedKey];
 secondBlocked.jobId = 3;
 secondBlocked.inFlight = true;
 distant::DistantRenderer::Completed secondFailure;
 secondFailure.key = secondBlockedKey;
 secondFailure.jobId = 3;
 secondFailure.chunkMask = 0xFFFFu;
 secondFailure.mesh.opaque.resize(4);
 renderer.shared_->completed.push_back(std::move(secondFailure));
 renderer.inFlight_ = 2;
 renderer.drainCompleted(1024);
 EXPECT_EQ(renderer.memoryStride_, 2);
 EXPECT_FALSE(renderer.coversColumn(20, 0));
 EXPECT_EQ(renderer.slab_.usedBytes(), pageBytes * 4);
 auto& replacementTile = renderer.tiles_.at(key);
 replacementTile.jobId = 2;
 replacementTile.inFlight = true;
 replacementTile.wantedStride = 2;
 renderer.inFlight_ = 1;
 distant::DistantRenderer::Completed replacement;
 replacement.key = key;
 replacement.jobId = 2;
 replacement.stride = 2;
 replacement.chunkMask = 0xFFFFu;
 replacement.mesh.opaque.resize(4);
 renderer.shared_->completed.push_back(std::move(replacement));
 renderer.drainCompleted(1024);
 ASSERT_TRUE(replacementTile.opaque.has_value());
 EXPECT_EQ(replacementTile.opaque->firstPage, 0u);
 EXPECT_EQ(replacementTile.opaque->pageCount, 1u);
 EXPECT_EQ(replacementTile.opaque->vertexCount, 4u);
 EXPECT_EQ(renderer.slab_.usedBytes(), pageBytes);
 EXPECT_TRUE(renderer.coversColumn(16, 0));
 EXPECT_TRUE(renderer.slab_.allocate(distant::DistantSlab::kPageVertices * 3).has_value());
 EXPECT_EQ(::glGetError(), 0u);
}
}
