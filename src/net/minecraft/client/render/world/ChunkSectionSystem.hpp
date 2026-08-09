#pragma once
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/world/TerrainScene.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::entity {
class LivingEntity;
}
namespace net::minecraft::block::entity {
class BlockEntity;
}
namespace net::minecraft::client::render {
class Frustum;
class ChunkCompilePipeline;
inline constexpr int kChunkSectionCountY = 8;
namespace world {
struct SectionPos {
 int x = 0;
 int y = 0;
 int z = 0;
 [[nodiscard]] bool operator==(const SectionPos& other) const noexcept {
  return x == other.x && y == other.y && z == other.z;
 }
};
struct SectionPosHash {
 [[nodiscard]] std::size_t operator()(const SectionPos& p) const noexcept {
  std::size_t h = static_cast<std::uint32_t>(p.x) * 0x9E3779B1u;
  h ^= static_cast<std::uint32_t>(p.z) * 0x85EBCA77u + (h << 6) + (h >> 2);
  h ^= static_cast<std::uint32_t>(p.y) * 0xC2B2AE3Du + (h << 6) + (h >> 2);
  return h;
 }
};
inline constexpr int kRegionSectionsX = 8;
inline constexpr int kRegionSectionsY = 4;
inline constexpr int kRegionSectionsZ = 8;
[[nodiscard]] inline SectionPos regionOf(const SectionPos& section) noexcept {
 using net::minecraft::util::math::MathHelper;
 return SectionPos{MathHelper::floorDiv(section.x, kRegionSectionsX),
                   MathHelper::floorDiv(section.y, kRegionSectionsY),
                   MathHelper::floorDiv(section.z, kRegionSectionsZ)};
}
} // namespace world
class ChunkSectionSystem {
 public:
 explicit ChunkSectionSystem(TerrainScene& scene) : scene_(scene) {
 }
 void setCompilePipeline(ChunkCompilePipeline& pipeline) noexcept {
  compilePipeline_ = &pipeline;
 }
 void clearSections();
 void reloadIfViewDistanceChanged();
 void resetSectionFrontier() noexcept {
  centerSectionX_ = std::numeric_limits<int>::min();
  centerSectionZ_ = std::numeric_limits<int>::min();
 }
 void setLastViewDistance(int viewDistance) noexcept {
  lastViewDistance = viewDistance;
 }
 void setLastRenderScale(float renderScale) noexcept {
  lastRenderScale = renderScale;
 }
 void pushCullState();
 void popCullState();
 void cullChunks(Frustum* culler, bool updateFrontier = true);
 void markDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
 void blockUpdate(int x, int y, int z);
 void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
 void chunkAvailable(int chunkX, int chunkZ);
 void chunkUnloaded(int chunkX, int chunkZ);
 void markChunkColumnLit(int chunkX, int chunkZ);
 void markAllChunksLit();
 void notifyAmbientDarknessChanged();
 void updateBlockEntity(int x, int y, int z, net::minecraft::block::entity::BlockEntity* blockEntity);
 [[nodiscard]] std::string getChunkDebugInfo() const;
 [[nodiscard]] bool empty() const noexcept {
  return sections_.empty();
 }
 [[nodiscard]] int renderRadiusChunks() const noexcept;
 [[nodiscard]] const std::vector<chunk::ChunkBuilder*>& visibleSections() const noexcept {
  return visibleSections_;
 }
 [[nodiscard]] const std::vector<chunk::ChunkBuilder*>& sectionsByPriority() const noexcept {
  return sectionsByPriority_;
 }
 void drainBorderRefresh();

 private:
  [[nodiscard]] const net::minecraft::entity::LivingEntity* frontierCamera() const;
  bool updateSectionFrontier();
 void drainPendingColumns();
 void createColumn(int sectionX, int sectionZ);
 void removeColumn(int sectionX, int sectionZ);
 void enqueueColumn(int sectionX, int sectionZ);
 void rebuildSectionOrder(const net::minecraft::Vec3d& camPos);
 void updateDebugCounts();
 void applyOcclusionCulling(Frustum* culler, const net::minecraft::Vec3d& camPos, double bypassSq);
 [[nodiscard]] chunk::ChunkBuilder* sectionAt(int sectionX, int sectionY, int sectionZ);
 TerrainScene& scene_;
 ChunkCompilePipeline* compilePipeline_ = nullptr;
 std::unordered_map<world::SectionPos, std::shared_ptr<chunk::ChunkBuilder>, world::SectionPosHash> sections_{};
 std::unordered_map<world::SectionPos, std::unique_ptr<chunk::TerrainRegion>, world::SectionPosHash> regions_{};
 std::vector<chunk::ChunkBuilder*> visibleSections_{};
 std::vector<chunk::ChunkBuilder*> sectionsByPriority_{};
 std::vector<chunk::ChunkBuilder*> savedVisibleSections_{};
 bool cullStateSaved_ = false;
 std::deque<world::SectionPos> pendingColumns_{};
 std::unordered_set<world::SectionPos, world::SectionPosHash> pendingSet_{};
 std::unordered_set<world::SectionPos, world::SectionPosHash> pendingBorderRefresh_{};
 std::unordered_set<world::SectionPos, world::SectionPosHash> pendingLit_{};
 int centerSectionX_ = std::numeric_limits<int>::min();
 int centerSectionZ_ = std::numeric_limits<int>::min();
 int lastViewDistance = -1;
 float lastRenderScale = -1.0f;
 int chunkCount = 0;
 int invisibleChunkCount = 0;
 int compiledChunkCount = 0;
 int emptyChunkCount = 0;
  int occlusionStamp_ = 0;
  std::vector<chunk::ChunkBuilder*> occlusionQueue_{};
  int meshOrderStamp_ = 0;
  bool sectionsChanged_ = true;
};
} // namespace net::minecraft::client::render
