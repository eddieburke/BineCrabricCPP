#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/render/distant/DistantTile.hpp"
namespace net::minecraft {
class BiomeSource;
class Chunk;
class World;
}
namespace net::minecraft::client::option {
struct RenderSettings;
}
namespace net::minecraft::client::texture {
class TextureManager;
}
namespace net::minecraft::util::concurrent {
class WorkerPool;
}
namespace net::minecraft::client::render {
class Frustum;
class Pipeline;
class ChunkSectionSystem;
struct PackInstance;
namespace distant {
// see LOD_DESIGN.md §4, §6, §7
class DistantRenderer {
 public:
 DistantRenderer();
 ~DistantRenderer();
 DistantRenderer(const DistantRenderer&) = delete;
 DistantRenderer& operator=(const DistantRenderer&) = delete;
 void reset();
 void markChunkDirty(int chunkX, int chunkZ, bool discardPending = false);
 void update(World& world, client::texture::TextureManager& textures, const ChunkSectionSystem& sections,
             const option::RenderSettings& settings,
             double eyeX, double eyeY, double eyeZ);
 void renderOpaque(Pipeline& pipeline, const option::RenderSettings& settings, const Frustum* culler);
 void renderWater(Pipeline& pipeline, const option::RenderSettings& settings, const Frustum* culler);
 [[nodiscard]] std::string debugInfo() const;
 [[nodiscard]] bool coversColumn(int chunkX, int chunkZ) const noexcept;
 std::uint64_t coverageRevision = 0;
 std::uint64_t coverageLossRevision = 0;

 private:
 struct TileKey {
  int x = 0;
  int z = 0;
  [[nodiscard]] bool operator==(const TileKey& other) const noexcept {
   return x == other.x && z == other.z;
  }
 };
 struct TileKeyHash {
  [[nodiscard]] std::size_t operator()(const TileKey& key) const noexcept {
   return static_cast<std::size_t>(static_cast<std::uint32_t>(key.x) * 0x9E3779B1u) ^
          static_cast<std::size_t>(static_cast<std::uint32_t>(key.z) * 0x85EBCA77u);
  }
 };
 struct Tile {
  int stride = 0;
  int wantedStride = 1;
  std::uint64_t generation = 0;
  std::uint64_t minimumGeneration = 0;
  std::uint64_t builtGeneration = 0;
  bool inFlight = false;
  std::uint64_t jobId = 0;
  std::uint16_t chunkMask = 0;
  int retryFrame = 0;
  std::optional<DistantSlab::Allocation> opaque;
  std::optional<DistantSlab::Allocation> water;
  int minY = 0;
  int maxY = 0;
  std::uint32_t seenFrame = 0;
 };
 struct Completed {
  TileKey key;
  int stride = 1;
  std::uint64_t generation = 0;
  std::uint64_t jobId = 0;
  TileMesh mesh;
  std::uint16_t chunkMask = 0;
 };
 static constexpr int kPinSide = kTileChunks + 2;
 struct PinnedChunks {
  int baseX = 0;
  int baseZ = 0;
  std::array<Chunk*, kPinSide * kPinSide> chunks{};
  ~PinnedChunks() { release(); }
  void release() noexcept;
 };
 struct Shared {
  std::mutex mutex;
  std::vector<Completed> completed;
 };
 void releaseTile(Tile& tile);
 void drainCompleted(std::size_t uploadBudget);
 void draw(Pipeline& pipeline, const option::RenderSettings& settings, const Frustum* culler, bool water);
 void ensureDepthTextures(int width, int height);
 void updateNativeRadius(const ChunkSectionSystem& sections, int requestedRadius);
 void collectBuildCandidates(std::size_t wanted = 4);
 bool updateWanted(const option::RenderSettings& settings, double eyeX, double eyeZ);
 [[nodiscard]] static int meshPriority(int distance, bool replacement);
 DistantSlab slab_;
 bool slabReady_ = false;
 BlockPalette palette_;
 std::optional<std::uint64_t> resourceGeneration_;
 std::unordered_map<TileKey, Tile, TileKeyHash> tiles_;
 std::shared_ptr<Shared> shared_ = std::make_shared<Shared>();
 std::vector<std::pair<int, TileKey>> wanted_;
 std::vector<std::pair<int, TileKey>> buildCandidates_;
 net::minecraft::util::concurrent::WorkerPool* pool_ = nullptr;
 std::weak_ptr<void> poolLifetime_;
 std::shared_ptr<BiomeSource> biomes_;
 std::uint32_t frame_ = 0;
 int inFlight_ = 0;
 int memoryStride_ = 1;
 int nativeRadius_ = -1;
 bool wantedValid_ = false;
 int wantedCellX_ = 0;
 int wantedCellZ_ = 0;
 int wantedNativeRadius_ = -1;
 int wantedRequestedRadius_ = -1;
 float wantedViewBlocks_ = 0;
 int wantedQuality_ = -1;
 int wantedMemoryStride_ = 0;
 std::uint64_t nextJobId_ = 0;
 double eyeX_ = 0.0;
 double eyeY_ = 0.0;
 double eyeZ_ = 0.0;
 int drawnLastFrame_ = 0;
 gl::GlTexture depth_[2];
 int depthWidth_ = 0;
 int depthHeight_ = 0;
};
} // namespace distant
} // namespace net::minecraft::client::render
