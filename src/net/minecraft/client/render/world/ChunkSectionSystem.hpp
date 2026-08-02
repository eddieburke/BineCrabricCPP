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
namespace net::minecraft {
class World;
}
namespace net::minecraft::entity {
class LivingEntity;
}
namespace net::minecraft::block::entity {
class BlockEntity;
}
namespace net::minecraft::client::render {
class FrustumCuller;
class WorldRenderer;
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
} // namespace world
class ChunkSectionSystem {
 public:
  explicit ChunkSectionSystem(WorldRenderer& facade) : facade_(facade) {
  }
  void clearSections();
  void reloadIfViewDistanceChanged();
  void resetSectionFrontier() noexcept {
   centerSectionX_ = std::numeric_limits<int>::min();
   centerSectionZ_ = std::numeric_limits<int>::min();
  }
  void setRenderRadius(int radius) noexcept {
   renderRadiusChunks_ = radius;
  }
  void setLastViewDistance(int viewDistance) noexcept {
   lastViewDistance = viewDistance;
  }
  void setLastRenderScale(float renderScale) noexcept {
   lastRenderScale = renderScale;
  }
  void pushCullState();
  void popCullState();
  void cullChunks(FrustumCuller* culler, float tickDelta, bool updateFrontier = true);
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
  [[nodiscard]] int renderRadiusChunks() const noexcept {
   return renderRadiusChunks_;
  }
  [[nodiscard]] int ringOf(int sectionX, int sectionZ) const noexcept;
  [[nodiscard]] int ringOf(const chunk::ChunkBuilder& chunk) const noexcept;
  [[nodiscard]] const std::vector<std::vector<chunk::ChunkBuilder*>>& visibleDrawRings() const noexcept {
   return visibleDrawRings_;
  }
  [[nodiscard]] const std::vector<std::unordered_set<chunk::ChunkBuilder*>>& drawRings() const noexcept {
   return drawRings_;
  }
   void drainBorderRefresh();
   [[nodiscard]] bool columnPendingLit(int sectionX, int sectionZ) const noexcept {
    return pendingLit_.contains(world::SectionPos{sectionX, 0, sectionZ});
   }

 private:
  [[nodiscard]] const net::minecraft::entity::LivingEntity* frontierCamera() const;
  void updateSectionFrontier();
  void drainPendingColumns();
  void createColumn(int sectionX, int sectionZ);
  void removeColumn(int sectionX, int sectionZ);
  void enqueueColumn(int sectionX, int sectionZ);
  void rebuildDrawRings();
  void rebuildVisibleDrawRings();
  void applyOcclusionCulling();
  [[nodiscard]] chunk::ChunkBuilder* sectionAt(int sectionX, int sectionY, int sectionZ);
  WorldRenderer& facade_;
  std::unordered_map<world::SectionPos, std::unique_ptr<chunk::ChunkBuilder>, world::SectionPosHash> sections_{};
  std::vector<chunk::ChunkBuilder*> sectionList_{};
  std::vector<std::unordered_set<chunk::ChunkBuilder*>> drawRings_{};
  std::vector<std::vector<chunk::ChunkBuilder*>> visibleDrawRings_{};
  // Scratch for pushCullState/popCullState. Kept resident so the inner vectors
  // retain their capacity across frames.
  std::vector<std::vector<chunk::ChunkBuilder*>> savedVisibleDrawRings_{};
  bool cullStateSaved_ = false;
  std::deque<world::SectionPos> pendingColumns_{};
  std::unordered_set<world::SectionPos, world::SectionPosHash> pendingSet_{};
   // Chunk columns that arrived since the last compileChunks pass. Drained once
   // per frame so a burst of arrivals refreshes each shared border exactly once
   // instead of once per arriving neighbour. Keyed with y == 0.
   std::unordered_set<world::SectionPos, world::SectionPosHash> pendingBorderRefresh_{};
   // Columns whose first mesh is held until their lighting drains. Keyed y == 0.
   std::unordered_set<world::SectionPos, world::SectionPosHash> pendingLit_{};
  int centerSectionX_ = std::numeric_limits<int>::min();
  int centerSectionZ_ = std::numeric_limits<int>::min();
  int renderRadiusChunks_ = 0;
  int lastViewDistance = -1;
  float lastRenderScale = -1.0f;
  int chunkCount = 0;
  int invisibleChunkCount = 0;
  int compiledChunkCount = 0;
  int emptyChunkCount = 0;
  int occlusionStamp_ = 0;
  std::vector<chunk::ChunkBuilder*> occlusionQueue_{};
};
} // namespace net::minecraft::client::render
