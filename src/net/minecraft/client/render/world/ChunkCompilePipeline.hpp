#pragma once
#include <memory>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
namespace net::minecraft::entity {
class LivingEntity;
}
namespace net::minecraft::client::render {
class WorldRenderer;
// Owns the mesh build/upload pipeline of the world renderer: the dirty-section
// sets, the retiring queue, the shared mesh scheduler and region pool, and the
// per-frame upload/capture FrameBudget slicing in compileChunks.
class ChunkCompilePipeline {
 public:
  explicit ChunkCompilePipeline(WorldRenderer& facade) : facade_(facade) {
  }
  bool compileChunks(net::minecraft::entity::LivingEntity& camera, bool force);
  void enqueueDirtyChunk(chunk::ChunkBuilder* chunk);
  void noteNearDirty(chunk::ChunkBuilder* chunk);
  bool startMeshJob(chunk::ChunkBuilder* chunk,
                    bool nearLane,
                    int priority,
                    const client::option::RenderSettings& resolvedOpts,
                    bool fancyGraphics);
  void sweepRetiring();
  void retireOrFreeSection(std::unique_ptr<chunk::ChunkBuilder> section);
  void cancelAll() {
   meshScheduler_.cancelAll();
   pendingMeshUploads_.clear();
  }
  void clearDirtyTracking() {
   dirtyChunks_.clear();
   nearDirtyChunks_.clear();
  }
  void clearRegionPool() {
   if(retiring_.empty()) {
    regionManager_.clear();
   }
  }
  chunk::ChunkRegionManager& regionManager() {
   return regionManager_;
  }

 private:
  WorldRenderer& facade_;
  std::unordered_set<chunk::ChunkBuilder*> dirtyChunks_{};
  std::unordered_set<chunk::ChunkBuilder*> nearDirtyChunks_{};
  std::vector<std::unique_ptr<chunk::ChunkBuilder>> retiring_{};
  chunk::ChunkRegionManager regionManager_{};
  chunk::ChunkMeshScheduler meshScheduler_{};
  std::vector<std::shared_ptr<chunk::ChunkMeshJob>> pendingMeshUploads_{};
};
} // namespace net::minecraft::client::render
