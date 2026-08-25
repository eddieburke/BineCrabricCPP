#pragma once
#include <cstdint>
#include <deque>
#include <limits>
#include <array>
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
// Sections only ever exist as whole columns, and a column key's y is always 0, so the
// column is the unit that is keyed and the section index is a plain array offset.
struct ColumnPos {
 int x = 0;
 int z = 0;
 [[nodiscard]] bool operator==(const ColumnPos& other) const noexcept {
  return x == other.x && z == other.z;
 }
};
struct ColumnPosHash {
 [[nodiscard]] std::size_t operator()(const ColumnPos& p) const noexcept {
  std::size_t h = static_cast<std::uint32_t>(p.x) * 0x9E3779B1u;
  h ^= static_cast<std::uint32_t>(p.z) * 0x85EBCA77u + (h << 6) + (h >> 2);
  return h;
 }
};
using SectionColumn = std::array<std::shared_ptr<chunk::ChunkBuilder>, kChunkSectionCountY>;
inline constexpr int kRegionSectionsX = 8;
inline constexpr int kRegionSectionsY = 4;
inline constexpr int kRegionSectionsZ = 8;
[[nodiscard]] inline SectionPos regionOf(const SectionPos& section) noexcept {
 using net::minecraft::util::math::MathHelper;
 return SectionPos{MathHelper::floorDiv(section.x, kRegionSectionsX),
                   MathHelper::floorDiv(section.y, kRegionSectionsY),
                   MathHelper::floorDiv(section.z, kRegionSectionsZ)};
}
using RegionMap =
    std::unordered_map<SectionPos, std::unique_ptr<chunk::TerrainRegion>, SectionPosHash>;
struct OcclusionQueueEntry {
 chunk::ChunkBuilder* section = nullptr;
 int entryFace = -1;
};
// The culling walks, as free functions over exactly what they read. They came out of
// ChunkSectionSystem so a benchmark can drive the same code the frame does; standing
// up the section system needs a live World, which culling has nothing to do with.
//
// Region-first: one plane test rejects up to kRegionSectionsX*Y*Z sections at once.
void cullByFrustum(const RegionMap& regions,
                   const Frustum& culler,
                   int stamp,
                   std::vector<chunk::ChunkBuilder*>& out);
// Vanilla's visibility graph: breadth-first from the camera's section, following only
// the face pairs its geometry lets you see through. `queue` is caller scratch.
void cullByOcclusionWalk(chunk::ChunkBuilder* start,
                         const Frustum& culler,
                         int stamp,
                         std::vector<OcclusionQueueEntry>& queue,
                         std::vector<chunk::ChunkBuilder*>& out);
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
 void setLastViewDistance(int viewDistance) noexcept {
  lastViewDistance = viewDistance;
 }
 // The nested shadow/portal pass culls into its own list so it cannot clobber the
 // one the outer pass still has to draw from. There is exactly one nesting level
 // (shadowmap::update only runs off the shadow pass), so this is a flag -- not the
 // stack push/pop implied. Nothing was ever saved, and pop's "already popped" guard
 // only skipped re-clearing a flag that was already clear.
 void useScopedVisibleSections(bool scoped) noexcept {
  scopedCull_ = scoped;
  if(scoped) {
   scopedVisibleSections_.clear();
  }
 }
 void cullChunks(Frustum* culler, bool updateGraph = true);
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
  return columns_.empty();
 }
 [[nodiscard]] const std::vector<chunk::ChunkBuilder*>& visibleSections() const noexcept {
  return scopedCull_ ? scopedVisibleSections_ : regularVisibleSections_;
 }
 // Bumped once per cull. ChunkBuilder::visibleIn compares against it, which is what
 // lets cullChunks skip the old full-world sweep that cleared a visibility flag on
 // every resident section every frame.
 [[nodiscard]] int frustumStamp() const noexcept {
  return frustumStamp_;
 }
 void drainBorderRefresh();

 private:
 void lightColumn(world::SectionColumn& column);
 void drainPendingColumns();
 void createColumn(int sectionX, int sectionZ);
 void removeColumn(int sectionX, int sectionZ);
 void enqueueColumn(int sectionX, int sectionZ);
 void updateDebugCounts();
 // False when the camera section is absent, so the caller falls back to the plain
 // frustum sweep; nothing has been pushed to the visible list in that case.
 bool applyOcclusionCulling(const Frustum& culler, const net::minecraft::Vec3d& camPos, int stamp);
 [[nodiscard]] std::vector<chunk::ChunkBuilder*>& currentVisibleSections() noexcept {
  return scopedCull_ ? scopedVisibleSections_ : regularVisibleSections_;
 }
 [[nodiscard]] chunk::ChunkBuilder* sectionAt(int sectionX, int sectionY, int sectionZ);
 TerrainScene& scene_;
 ChunkCompilePipeline* compilePipeline_ = nullptr;
 std::unordered_map<world::ColumnPos, world::SectionColumn, world::ColumnPosHash> columns_{};
 world::RegionMap regions_{};
 std::vector<chunk::ChunkBuilder*> regularVisibleSections_{};
 std::vector<chunk::ChunkBuilder*> scopedVisibleSections_{};
 bool scopedCull_ = false;
 std::deque<world::ColumnPos> pendingColumns_{};
 std::unordered_set<world::ColumnPos, world::ColumnPosHash> pendingSet_{};
 std::unordered_set<world::ColumnPos, world::ColumnPosHash> pendingBorderRefresh_{};
 std::unordered_set<world::ColumnPos, world::ColumnPosHash> pendingLit_{};
 int lastViewDistance = -1;
 int chunkCount = 0;
 int invisibleChunkCount = 0;
 int compiledChunkCount = 0;
 int emptyChunkCount = 0;
 int builtChunkCount = 0;
 int visibleBuiltChunkCount = 0;
 int dirtyChunkCount = 0;
 int inFlightChunkCount = 0;
 int lightingPendingChunkCount = 0;
 int debugCountCooldown_ = 0;
 // One stamp for the whole cull: the occlusion walk and the frustum answer are
 // recorded on the same pass over the same sections, so two counters could only
 // ever disagree.
 int frustumStamp_ = 0;
  std::vector<world::OcclusionQueueEntry> occlusionQueue_{};
};
} // namespace net::minecraft::client::render
