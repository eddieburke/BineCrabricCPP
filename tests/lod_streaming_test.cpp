#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <future>
#include <filesystem>
#include <limits>
#include <memory>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#define private public
#include "net/minecraft/client/render/distant/DistantRenderer.hpp"
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#undef private
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"

namespace net::minecraft::test {
namespace {
using namespace client::render;
using net::minecraft::world::chunk::ChunkCache;

class LodWorld : public World {
 public:
 using World::World;
 using World::setChunkCache;
 using World::chunkGeneratorSource_;
};

class TemporaryClient : public client::Minecraft {
 public:
 using client::Minecraft::Minecraft;
 void handleCrash(const util::crash::CrashReport&) override {}
};

class ObservedGenerator : public OverworldChunkGenerator {
 public:
 ObservedGenerator(World* world, std::atomic<bool>& destroyed)
     : OverworldChunkGenerator(world, world->getSeed()), destroyed_(destroyed) {}
 ~ObservedGenerator() override { destroyed_.store(true, std::memory_order_release); }
 private:
 std::atomic<bool>& destroyed_;
};

class LodStorage : public ChunkStorage {
 public:
 std::atomic<int> reads{0};
 std::atomic<int> attachedReads{0};
 std::atomic<int> writes{0};
 Chunk loadChunk(World* owner, int x, int z) override {
  ++reads;
  if(owner != nullptr) ++attachedReads;
  Chunk chunk(owner, x, z);
  chunk.terrainPopulated = true;
  chunk.heightmap.fill(64);
  std::fill(chunk.skyLight.bytes.begin(), chunk.skyLight.bytes.end(), 0xFFu);
  for(int bx = 0; bx < 16; ++bx) {
   for(int bz = 0; bz < 16; ++bz) {
    const auto base = (bx << 11) | (bz << 7);
    std::fill_n(chunk.blocks.begin() + base, 63, 1u);
    chunk.blocks[static_cast<std::size_t>(base + 63)] = 2;
   }
  }
  chunk.populateHeightMapOnly();
  return chunk;
 }
 void saveChunk(World*, Chunk&) override { ++writes; }
 void saveEntities(World*, Chunk&) override { ++writes; }
 void tick() override {}
 void flush() override {}
};

class DistantStreamingGl : public ::testing::Test {
 protected:
 void SetUp() override {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  window_ = glfwCreateWindow(32, 32, "lod-streaming", nullptr, nullptr);
  ASSERT_NE(window_, nullptr);
  glfwMakeContextCurrent(window_);
  client::gl::GLCore::ensureLoaded();
 }
 void TearDown() override {
  core::releaseGlResources();
  glfwMakeContextCurrent(nullptr);
  if(window_ != nullptr) glfwDestroyWindow(window_);
 }
 GLFWwindow* window_ = nullptr;
};
}

TEST(LodStreaming, SavedChunkReadsDoNotAdoptChunksOrPublishLighting) {
 LodWorld worldInstance("lod-read", 12345ULL);
 auto storage = std::make_unique<LodStorage>();
 auto* recorded = storage.get();
 std::promise<std::unique_ptr<Chunk>> completed;
 auto result = completed.get_future();
 {
  ChunkCache cache(&worldInstance, std::move(storage), nullptr);
  cache.setActiveRadius(2);
  EXPECT_TRUE(cache.readChunkDataAsync(
      [&completed](const ChunkCache::ChunkDataLoader& load) { completed.set_value(load(-1024, 2048)); },
      4096, &completed, [&completed] { completed.set_value(nullptr); }));
  ASSERT_EQ(result.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  const auto chunk = result.get();
  ASSERT_NE(chunk, nullptr);
  EXPECT_EQ(chunk->world, nullptr);
  EXPECT_EQ(chunk->x, -1024);
  EXPECT_EQ(chunk->z, 2048);
  EXPECT_EQ(chunk->getBlockId(0, 63, 0), 2);
  EXPECT_FALSE(chunk->loaded);
  EXPECT_EQ(recorded->reads.load(), 1);
  EXPECT_EQ(recorded->attachedReads.load(), 0);
  EXPECT_EQ(recorded->writes.load(), 0);
  EXPECT_EQ(cache.loadedChunkCount(), 0u);
  EXPECT_EQ(cache.pendingAsyncLoadCount(), 0u);
  EXPECT_FALSE(worldInstance.hasPendingLightingUpdates());
 }
}

TEST(LodStreaming, DestroyedClientsCannotRemainBoundToTheRenderer) {
 auto first = std::make_unique<TemporaryClient>(64, 64, false);
 auto second = std::make_unique<TemporaryClient>(64, 64, false);
 first.reset();
 EXPECT_EQ(client::Minecraft::INSTANCE, second.get());
 second.reset();
 EXPECT_EQ(client::Minecraft::INSTANCE, nullptr);
}

TEST(LodStreaming, GeneratedChunksMatchTheWorldGeneratorWithoutEnteringResidency) {
 LodWorld parent("lod-generation", 12345ULL);
 for(const int dimensionId : {0, -1, 1}) {
  World worldInstance(&parent, Dimension::fromId(dimensionId));
  auto generator = worldInstance.dimension->createChunkGeneratorFromSeed(worldInstance.getSeed());
  std::promise<std::unique_ptr<Chunk>> completed;
  auto result = completed.get_future();
  ChunkCache cache(&worldInstance, nullptr, generator.get());
  EXPECT_TRUE(cache.readChunkDataAsync(
      [&completed](const ChunkCache::ChunkDataLoader& load) { completed.set_value(load(1000, -1000)); },
      4096, &completed, [&completed] { completed.set_value(nullptr); }));
  ASSERT_EQ(result.wait_for(std::chrono::seconds(20)), std::future_status::ready) << dimensionId;
  const auto generated = result.get();
  ASSERT_NE(generated, nullptr) << dimensionId;
  Chunk reference = std::move(generator->getChunk(1000, -1000));
  EXPECT_EQ(generated->blocks, reference.blocks) << dimensionId;
  EXPECT_EQ(generated->meta.bytes, reference.meta.bytes) << dimensionId;
  EXPECT_EQ(generated->heightmap, reference.heightmap) << dimensionId;
  EXPECT_EQ(generated->skyLight.bytes, reference.skyLight.bytes) << dimensionId;
  EXPECT_EQ(generated->world, nullptr);
  EXPECT_EQ(cache.loadedChunkCount(), 0u);
  EXPECT_FALSE(worldInstance.hasPendingLightingUpdates()) << dimensionId;
 }
}

TEST(LodStreaming, RemoteWorldsRejectDetachedTerrainReads) {
 ClientWorld worldInstance(nullptr, 12345ULL, 0);
 auto* cache = dynamic_cast<ChunkCache*>(worldInstance.getChunkSource());
 ASSERT_NE(cache, nullptr);
 bool ran = false;
 bool cancelled = false;
 EXPECT_FALSE(cache->readChunkDataAsync([&](const auto&) { ran = true; }, 4096,
                                        &ran, [&] { cancelled = true; }));
 EXPECT_FALSE(ran);
 EXPECT_TRUE(cancelled);
 EXPECT_EQ(cache->loadedChunkCount(), 0u);
}

TEST(LodStreaming, CacheDestructionWaitsForDetachedReads) {
 LodWorld worldInstance("lod-lifetime", 12345ULL);
 std::promise<void> entered;
 auto started = entered.get_future();
 std::promise<void> release;
 auto released = release.get_future().share();
 auto cache = std::make_unique<ChunkCache>(&worldInstance, std::make_unique<LodStorage>(), nullptr);
 EXPECT_TRUE(cache->readChunkDataAsync(
     [&entered, released](const auto& load) {
      entered.set_value();
      released.wait();
      EXPECT_NE(load(100, 100), nullptr);
     },
     4096, &entered, [] {}));
 const auto state = started.wait_for(std::chrono::seconds(10));
 if(state != std::future_status::ready) {
  release.set_value();
  cache.reset();
  FAIL() << "Detached read did not start";
 }
 auto destroyed = std::async(std::launch::async, [cache = std::move(cache)]() mutable { cache.reset(); });
 EXPECT_EQ(destroyed.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);
 release.set_value();
 EXPECT_EQ(destroyed.wait_for(std::chrono::seconds(10)), std::future_status::ready);
 destroyed.get();
}

TEST(LodStreaming, WorldKeepsItsGeneratorAliveUntilDetachedReadsFinish) {
 std::atomic<bool> generatorDestroyed{false};
 auto worldInstance = std::make_unique<LodWorld>("lod-world-lifetime", 12345ULL);
 worldInstance->setChunkCache(nullptr);
 worldInstance->chunkGeneratorSource_ = std::make_unique<ObservedGenerator>(worldInstance.get(), generatorDestroyed);
 auto owned = std::make_unique<ChunkCache>(worldInstance.get(), std::make_unique<LodStorage>(),
                                          worldInstance->chunkGeneratorSource_.get());
 auto* cache = owned.get();
 worldInstance->setChunkCache(std::move(owned));
 std::promise<void> entered;
 auto started = entered.get_future();
 std::promise<void> release;
 auto released = release.get_future().share();
 EXPECT_TRUE(cache->readChunkDataAsync(
     [&entered, released](const auto& load) {
      entered.set_value();
      released.wait();
      EXPECT_NE(load(100, 100), nullptr);
     },
     4096, &entered, [] {}));
 if(started.wait_for(std::chrono::seconds(10)) != std::future_status::ready) {
  release.set_value();
  worldInstance.reset();
  FAIL() << "Detached read did not start";
 }
 auto destroyed = std::async(std::launch::async,
                              [worldInstance = std::move(worldInstance)]() mutable { worldInstance.reset(); });
 EXPECT_EQ(destroyed.wait_for(std::chrono::milliseconds(30)), std::future_status::timeout);
 EXPECT_FALSE(generatorDestroyed.load(std::memory_order_acquire));
 release.set_value();
 EXPECT_EQ(destroyed.wait_for(std::chrono::seconds(10)), std::future_status::ready);
 destroyed.get();
 EXPECT_TRUE(generatorDestroyed.load(std::memory_order_acquire));
}

TEST(LodStreaming, CachedGridCoversTheLongHorizonAtEverySubchunkCorner) {
 client::option::GameOptions options;
 options.renderScale = 32.0f;
 const auto settings = client::option::renderSettings(options);
 TerrainScene scene;
 ChunkSectionSystem sections(scene);
 distant::DistantRenderer renderer;
 renderer.nativeRadius_ = settings.nativeChunkRadius;
 constexpr double pi = 3.14159265358979323846;
 for(const int cameraChunk : {-5, -1, 0, 5}) {
  renderer.eyeX_ = cameraChunk * 16.0;
  renderer.eyeZ_ = cameraChunk * 16.0;
  ++renderer.frame_;
  renderer.updateTileGrid(sections, settings);
  const distant::DistantRenderer::TileKey probe{(cameraChunk >> 2) + 127, cameraChunk >> 2};
  const auto stamp = renderer.tiles_.at(probe).seenFrame;
  const auto count = renderer.wanted_.size();
  EXPECT_GT(count, 50000u);
  for(const double dx : {0.0, 7.99, 15.99}) {
   for(const double dz : {0.0, 7.99, 15.99}) {
    renderer.eyeX_ = cameraChunk * 16.0 + dx;
    renderer.eyeZ_ = cameraChunk * 16.0 + dz;
    ++renderer.frame_;
    renderer.updateTileGrid(sections, settings);
    EXPECT_EQ(renderer.wanted_.size(), count);
    EXPECT_EQ(renderer.tiles_.at(probe).seenFrame, stamp);
    for(int angle = 0; angle < 360; angle += 3) {
     const double radians = angle * pi / 180.0;
     const double range = settings.renderDistance.blocks - 0.01;
     const int tileX = static_cast<int>(std::floor((renderer.eyeX_ + range * std::cos(radians)) / distant::kTileBlocks));
     const int tileZ = static_cast<int>(std::floor((renderer.eyeZ_ + range * std::sin(radians)) / distant::kTileBlocks));
     ASSERT_TRUE(renderer.tiles_.contains({tileX, tileZ})) << cameraChunk << ' ' << dx << ' ' << dz << ' ' << angle;
    }
   }
  }
 }
}

TEST(LodStreaming, GridChangesDetailWhenQualityAndMemoryPressureChange) {
 client::option::GameOptions options;
 options.renderScale = 8.0f;
 auto settings = client::option::renderSettings(options);
 TerrainScene scene;
 ChunkSectionSystem sections(scene);
 distant::DistantRenderer renderer;
 renderer.nativeRadius_ = settings.nativeChunkRadius;
 renderer.eyeX_ = renderer.eyeZ_ = 8.0;
 ++renderer.frame_;
 renderer.updateTileGrid(sections, settings);
 const distant::DistantRenderer::TileKey key{10, 0};
 EXPECT_EQ(renderer.tiles_.at(key).wantedStride, 2);
 settings.lodQuality = 0;
 ++renderer.frame_;
 renderer.updateTileGrid(sections, settings);
 EXPECT_EQ(renderer.tiles_.at(key).wantedStride, 1);
 renderer.memoryStride_ = 16;
 ++renderer.frame_;
 renderer.updateTileGrid(sections, settings);
 EXPECT_GE(renderer.tiles_.at(key).wantedStride, 2);
 settings.renderDistance.blocks = 512.0f;
 ++renderer.frame_;
 renderer.updateTileGrid(sections, settings);
 EXPECT_FALSE(renderer.tiles_.contains(key));
}

TEST(LodStreaming, ReleasingAnOutOfRangeTileCancelsItsDetachedReads) {
 distant::DistantRenderer renderer;
 distant::DistantRenderer::Tile tile;
 tile.cancellation = std::make_shared<std::atomic<bool>>(false);
 const auto cancelled = tile.cancellation;
 renderer.releaseTile(tile);
 EXPECT_TRUE(cancelled->load());
}

TEST(LodStreaming, NewlyLoadedTerrainRefinesNearbyGeneratedTilesBeforeMoreDistantCoverage) {
 distant::DistantRenderer renderer;
 auto& refresh = renderer.tiles_[{4, 0}];
 refresh.stride = refresh.wantedStride = 1;
 refresh.chunkMask = 0xFFFFu;
 refresh.generation = 1;
 refresh.readyPinMask = 1u << 7;
 renderer.tiles_[{8, 0}] = {};
 renderer.wanted_.emplace_back(256, distant::DistantRenderer::TileKey{4, 0});
 renderer.wanted_.emplace_back(512, distant::DistantRenderer::TileKey{8, 0});
 renderer.collectBuildCandidates(1);
 ASSERT_EQ(renderer.buildCandidates_.size(), 1u);
 EXPECT_EQ(renderer.buildCandidates_.front().second.x, 4);
}

TEST(LodStreaming, CancellingQueuedReadsReleasesTheirCacheLifetime) {
 LodWorld worldInstance("lod-cancel", 12345ULL);
 auto cache = std::make_unique<ChunkCache>(&worldInstance, std::make_unique<LodStorage>(), nullptr);
 auto& pool = util::concurrent::ThreadCoordinator::instance().pool(util::concurrent::Domain::Compute);
 pool.drain();
 const auto previousLimit = pool.concurrencyLimit();
 util::concurrent::ThreadCoordinator::instance().setComputeThreadLimit(1);
 std::promise<void> entered;
 auto started = entered.get_future();
 std::promise<void> release;
 auto released = release.get_future().share();
 struct Restore {
  util::concurrent::WorkerPool& pool;
  unsigned limit;
  std::promise<void>& release;
  ~Restore() {
   release.set_value();
   pool.drain();
   util::concurrent::ThreadCoordinator::instance().setComputeThreadLimit(limit);
  }
 } restore{pool, previousLimit, release};
 pool.submit([&entered, released] { entered.set_value(); released.wait(); });
 ASSERT_EQ(started.wait_for(std::chrono::seconds(10)), std::future_status::ready);
 bool ran = false;
 bool cancelled = false;
 EXPECT_TRUE(cache->readChunkDataAsync([&](const auto&) { ran = true; }, 4096,
                                       &cancelled, [&] { cancelled = true; }));
 pool.cancelPending(&cancelled);
 EXPECT_TRUE(cancelled);
 EXPECT_FALSE(ran);
 cache.reset();
}

TEST_F(DistantStreamingGl, MissingDistantChunksReachTheGpuWithoutFullChunkResidency) {
 LodWorld worldInstance("lod-gpu", 12345ULL);
 auto storage = std::make_unique<LodStorage>();
 auto* recorded = storage.get();
 auto owned = std::make_unique<ChunkCache>(&worldInstance, std::move(storage), nullptr);
 auto* cache = owned.get();
 cache->setActiveRadius(2);
 worldInstance.setChunkCache(std::move(owned));
 client::option::GameOptions options;
 options.lodMemoryMiB = 64;
 const auto settings = client::option::renderSettings(options);
 client::texture::TextureManager textures(&options);
 TerrainScene scene;
 scene.world = &worldInstance;
 scene.settings = &settings;
 scene.options = &options;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 sections.setCompilePipeline(pipeline);
 pipeline.setSectionSystem(sections);
 distant::DistantRenderer renderer;
 renderer.resourceGeneration_ = textures.resourceGeneration();
 renderer.update(worldInstance, textures, sections, settings, 0.0, 70.0, 0.0);
 ASSERT_FALSE(renderer.wanted_.empty());
 const auto key = renderer.wanted_.front().second;
 auto& pool = util::concurrent::ThreadCoordinator::instance().pool(util::concurrent::Domain::Compute);
 pool.drain();
 renderer.drainCompleted(4u * 1024u * 1024u);
 EXPECT_TRUE(renderer.coversColumn(key.x * distant::kTileChunks, key.z * distant::kTileChunks));
 const auto& tile = renderer.tiles_.at(key);
 ASSERT_TRUE(tile.opaque.has_value());
 EXPECT_GT(tile.opaque->vertexCount, 0u);
 EXPECT_EQ(tile.chunkMask, 0xFFFFu);
 EXPECT_EQ(tile.builtPinMask, 0u);
 EXPECT_GT(recorded->reads.load(), 0);
 EXPECT_EQ(recorded->attachedReads.load(), 0);
 EXPECT_EQ(recorded->writes.load(), 0);
 EXPECT_EQ(cache->loadedChunkCount(), 0u);
 EXPECT_EQ(cache->pendingAsyncLoadCount(), 0u);
 EXPECT_LE(renderer.slab_.usedBytes(), renderer.slab_.capacityBytes());
 EXPECT_FALSE(worldInstance.hasPendingLightingUpdates());
 EXPECT_EQ(::glGetError(), 0u);
 auto& live = cache->loadChunk(key.x * distant::kTileChunks, key.z * distant::kTileChunks);
 ASSERT_TRUE(live.setBlock(0, 80, 0, 2));
 worldInstance.finishLightingUpdates();
 renderer.update(worldInstance, textures, sections, settings, 0.0, 70.0, 0.0);
 pool.drain();
 renderer.drainCompleted(4u * 1024u * 1024u);
 const auto& refined = renderer.tiles_.at(key);
 EXPECT_NE(refined.builtPinMask, 0u);
 EXPECT_EQ(refined.maxY, 81);
 EXPECT_EQ(cache->loadedChunkCount(), 1u);
 EXPECT_EQ(recorded->writes.load(), 0);
 EXPECT_EQ(::glGetError(), 0u);
 renderer.reset();
 sections.clearSections();
}

TEST_F(DistantStreamingGl, RealTerrainUploadsBeyondThePreviousMaximumViewDistance) {
 LodWorld worldInstance("lod-far-generation", 12345ULL);
 auto* cache = dynamic_cast<ChunkCache*>(worldInstance.getChunkSource());
 ASSERT_NE(cache, nullptr);
 cache->setActiveRadius(2);
 client::option::GameOptions options;
 options.renderScale = 32.0f;
 options.lodMemoryMiB = 64;
 const auto settings = client::option::renderSettings(options);
 client::texture::TextureManager textures(&options);
 TerrainScene scene;
 scene.world = &worldInstance;
 scene.settings = &settings;
 scene.options = &options;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 sections.setCompilePipeline(pipeline);
 pipeline.setSectionSystem(sections);
 distant::DistantRenderer renderer;
 renderer.resourceGeneration_ = textures.resourceGeneration();
 renderer.update(worldInstance, textures, sections, settings, 0.0, 70.0, 0.0);
 auto& pool = util::concurrent::ThreadCoordinator::instance().pool(util::concurrent::Domain::Compute);
 pool.drain();
 renderer.drainCompleted(64u * 1024u * 1024u);
 ASSERT_EQ(renderer.inFlight_, 0);
 const distant::DistantRenderer::TileKey key{120, 0};
 ASSERT_TRUE(renderer.tiles_.contains(key));
 for(auto& [position, tile] : renderer.tiles_) tile.retryFrame = std::numeric_limits<int>::max();
 renderer.tiles_.at(key).retryFrame = 0;
 renderer.update(worldInstance, textures, sections, settings, 0.0, 70.0, 0.0);
 pool.drain();
 renderer.drainCompleted(4u * 1024u * 1024u);
 const auto& tile = renderer.tiles_.at(key);
 EXPECT_TRUE(renderer.coversColumn(key.x * distant::kTileChunks, key.z * distant::kTileChunks));
 ASSERT_TRUE(tile.opaque.has_value());
 EXPECT_GT(tile.opaque->vertexCount, 0u);
 EXPECT_GT(tile.maxY, 0);
 EXPECT_EQ(tile.builtPinMask, 0u);
 EXPECT_EQ(cache->loadedChunkCount(), 0u);
 EXPECT_EQ(cache->pendingAsyncLoadCount(), 0u);
 EXPECT_FALSE(worldInstance.hasPendingLightingUpdates());
 EXPECT_EQ(::glGetError(), 0u);
 renderer.reset();
 sections.clearSections();
}

TEST_F(DistantStreamingGl, IceHasItsOwnGpuAllocationAndRetiresWithTheTile) {
 distant::DistantRenderer renderer;
 constexpr std::size_t pageBytes = distant::DistantSlab::kPageVertices * sizeof(distant::DhVertex);
 ASSERT_TRUE(renderer.slab_.create(pageBytes * 3));
 auto& tile = renderer.tiles_[{4, 0}];
 tile.inFlight = true;
 tile.jobId = 1;
 renderer.inFlight_ = 1;
 distant::DistantRenderer::Completed result;
 result.key = {4, 0};
 result.jobId = 1;
 result.chunkMask = 0xFFFFu;
 result.mesh.opaque.resize(4);
 result.mesh.water.resize(4);
 result.mesh.ice.resize(4);
 renderer.shared_->completed.push_back(std::move(result));
 renderer.drainCompleted(pageBytes * 3);
 ASSERT_TRUE(tile.opaque.has_value());
 ASSERT_TRUE(tile.water.has_value());
 ASSERT_TRUE(tile.ice.has_value());
 EXPECT_NE(tile.ice->firstPage, tile.opaque->firstPage);
 EXPECT_NE(tile.ice->firstPage, tile.water->firstPage);
 EXPECT_EQ(renderer.slab_.usedBytes(), pageBytes * 3);
 EXPECT_TRUE(renderer.coversColumn(16, 0));
 renderer.releaseTile(tile);
 EXPECT_EQ(renderer.slab_.usedBytes(), 0u);
 EXPECT_FALSE(tile.ice.has_value());
 EXPECT_EQ(::glGetError(), 0u);
}

TEST_F(DistantStreamingGl, IceBlendsOverTheBedWhenWaterRenderingIsDisabled) {
 const auto savedGlState = core::capturePassGlBits();
 client::option::GameOptions options;
 options.shaderPack.clear();
 auto settings = client::option::renderSettings(options);
 settings.renderWater = false;
 Pipeline shaderPipeline(std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR), &options, {});
 shaderPipeline.setDistantHorizons(true);
 shaderPipeline.prepareFrame(nullptr);
 ASSERT_TRUE(shaderPipeline.ensureSceneTargets(32, 32));
 shaderPipeline.setPipelinePhase(WorldPipelinePhase::World);
 auto* scenePack = shaderPipeline.distantScenePack();
 ASSERT_NE(scenePack, nullptr);
 ASSERT_NE(shaderPipeline.distantProgram(false), nullptr);
 ASSERT_NE(shaderPipeline.distantProgram(true), nullptr);
 FrameRenderCamera camera;
 camera.eyeX = camera.eyeZ = 32.0;
 camera.eyeY = 100.0;
 camera.orthographic = true;
 camera.orthoHalfWidth = camera.orthoHalfHeight = 16.0f;
 camera.viewUpY = 0.0f;
 camera.viewUpZ = -1.0f;
 camera.viewForwardY = -1.0f;
 camera.viewForwardZ = 0.0f;
 camera.farPlane = camera.dhFarPlane = 512.0f;
 camera.dhNearPlane = 16.0f;
 core::setCameraFrame(camera);
 core::setDrawCameraStateFromCamera(camera);
 core::setFogEnabled(false);
 PackUniformValues uniforms;
 uniforms.viewWidth = uniforms.viewHeight = 32.0f;
 uniforms.aspectRatio = 1.0f;
 buildCameraProjection(uniforms.gbufferProjection, camera);
 buildCameraProjectionInverse(uniforms.gbufferProjectionInverse, camera);
 buildCameraModelView(uniforms.gbufferModelView, camera);
 buildCameraModelViewInverse(uniforms.gbufferModelViewInverse, camera);
 shaderPipeline.setFrameUniforms(uniforms);
 distant::DistantRenderer renderer;
 renderer.eyeX_ = renderer.eyeZ_ = 32.0;
 renderer.eyeY_ = 100.0;
 renderer.nativeRadius_ = 0;
 renderer.slabReady_ = renderer.slab_.create(3u * distant::DistantSlab::kPageVertices * sizeof(distant::DhVertex));
 ASSERT_TRUE(renderer.slabReady_);
 auto& tile = renderer.tiles_[{0, 0}];
 tile.inFlight = true;
 tile.jobId = 1;
 renderer.inFlight_ = 1;
 renderer.wanted_.emplace_back(0, distant::DistantRenderer::TileKey{0, 0});
 distant::DistantRenderer::Completed result;
 result.key = {0, 0};
 result.jobId = 1;
 result.chunkMask = 0xFFFFu;
 result.mesh.minY = 60;
 result.mesh.maxY = 65;
 const auto plane = [](auto& vertices, int y, std::array<std::uint8_t, 4> color) {
  for(const auto [x, z] : {std::pair{0, 0}, std::pair{0, 64}, std::pair{64, 64}, std::pair{64, 0}}) {
   distant::DhVertex vertex;
   vertex.x = static_cast<std::uint16_t>(x);
   vertex.y = static_cast<std::uint16_t>(y);
   vertex.z = static_cast<std::uint16_t>(z);
   vertex.meta = 0x0F;
   vertex.normal = distant::DhNormal::PosY;
   std::copy(color.begin(), color.end(), vertex.color);
   vertices.push_back(vertex);
  }
 };
 plane(result.mesh.opaque, 60, {0, 255, 0, 255});
 plane(result.mesh.ice, 64, {0, 0, 255, 128});
 plane(result.mesh.water, 65, {255, 0, 0, 255});
 renderer.shared_->completed.push_back(std::move(result));
 renderer.drainCompleted(1024);
 TerrainScene scene;
 ChunkSectionSystem sections(scene);
 renderer.updateTransitionCoverage(sections);
 shaderPipeline.clearScene(1.0f, 0.0f, 0.0f);
 shaderPipeline.bindScene();
 core::depthMask(true);
 ::glClearDepth(1.0);
 ::glClear(GL_DEPTH_BUFFER_BIT);
 core::blendAlpha();
 renderer.renderOpaque(shaderPipeline, settings, nullptr);
 ASSERT_NE(scenePack->lodCoverageTexture, 0u);
 renderer.renderWater(shaderPipeline, settings, nullptr);
 std::array<unsigned char, 4> pixel{};
 ::glReadBuffer(0x8CE0);
 ::glReadPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
 EXPECT_EQ(pixel[0], 0);
 EXPECT_NEAR(pixel[1], 127, 1);
 EXPECT_NEAR(pixel[2], 128, 1);
 EXPECT_EQ(::glGetError(), 0u);
 renderer.reset();
 core::setActiveProgram(nullptr);
 core::setCameraFrame({});
 shaderPipeline.endScene();
 core::restorePassGlBits(savedGlState);
}
}
