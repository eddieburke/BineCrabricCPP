#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/culling/ShadowFrustum.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/concurrent/FrameBudget.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
namespace net::minecraft::client::render {
namespace {
template <class Fn>
void forEachSection(const world::RegionMap& regions, Fn&& fn) {
 for(const auto& entry : regions) {
  for(chunk::ChunkBuilder* chunk : entry.second->sections()) {
   fn(chunk);
  }
 }
}
// One plane test per region rejects up to kRegionSectionsX*Y*Z sections at once.
// The padding matches ChunkBuilder::kCullPadding so this box conservatively
// contains every section box inside the region -- reject here and no section in it
// could have passed.
[[nodiscard]] net::minecraft::Box regionCullingBox(const chunk::TerrainRegion& region) {
 constexpr double padding = 6.0;
 const double minX = static_cast<double>(region.originX()) - padding;
 const double minY = static_cast<double>(region.originY()) - padding;
 const double minZ = static_cast<double>(region.originZ()) - padding;
 return net::minecraft::Box(
     minX, minY, minZ,
     minX + static_cast<double>(world::kRegionSectionsX * chunk::kSectionBlocks) + 2.0 * padding,
     minY + static_cast<double>(world::kRegionSectionsY * chunk::kSectionBlocks) + 2.0 * padding,
     minZ + static_cast<double>(world::kRegionSectionsZ * chunk::kSectionBlocks) + 2.0 * padding);
}
constexpr int kFaceDirX[6] = {-1, 1, 0, 0, 0, 0};
constexpr int kFaceDirY[6] = {0, 0, -1, 1, 0, 0};
constexpr int kFaceDirZ[6] = {0, 0, 0, 0, -1, 1};
} // namespace
chunk::ChunkBuilder* ChunkSectionSystem::sectionAt(int sectionX, int sectionY, int sectionZ) {
 if(sectionY < 0 || sectionY >= kChunkSectionCountY) {
  return nullptr;
 }
 const auto it = columns_.find(world::ColumnPos{sectionX, sectionZ});
 return it == columns_.end() ? nullptr : it->second[static_cast<std::size_t>(sectionY)].get();
}
void ChunkSectionSystem::enqueueColumn(int sectionX, int sectionZ) {
 if(sectionAt(sectionX, 0, sectionZ) != nullptr) {
  return;
 }
 const world::ColumnPos key{sectionX, sectionZ};
 if(pendingSet_.insert(key).second) {
  pendingColumns_.push_back(key);
 }
}
void ChunkSectionSystem::createColumn(int sectionX, int sectionZ) {
 if(scene_.world == nullptr || scene_.world->getChunkSource() == nullptr ||
    !scene_.world->getChunkSource()->isChunkLoaded(sectionX, sectionZ)) {
  return;
 }
 const world::ColumnPos key{sectionX, sectionZ};
 const bool lightingReady = !pendingLit_.contains(key);
 ChunkSource* source = scene_.world->getChunkSource();
 Chunk* sourceChunk = source->getChunkIfLoaded(sectionX, sectionZ);
 if(sourceChunk == nullptr) {
  return;
 }
 world::SectionColumn& column = columns_[key];
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  if(column[static_cast<std::size_t>(sectionY)] != nullptr) {
   continue;
  }
  const world::SectionPos pos{sectionX, sectionY, sectionZ};
  auto builder = std::make_shared<chunk::ChunkBuilder>(scene_.world,
                                                       scene_.blockEntities,
                                                       sectionX * chunk::kSectionBlocks,
                                                       sectionY * chunk::kSectionBlocks,
                                                       sectionZ * chunk::kSectionBlocks);
  const bool hasBlocks = sourceChunk->sectionHasBlocks(sectionY);
  builder->lightingReady = lightingReady || !hasBlocks;
  if(hasBlocks) {
   builder->invalidate();
  } else {
   builder->built = true;
  }
  chunk::ChunkBuilder* raw = builder.get();
  const world::SectionPos regionPos = world::regionOf(pos);
  auto& region = regions_[regionPos];
  if(region == nullptr) {
   region = std::make_unique<chunk::TerrainRegion>(
       regionPos.x * world::kRegionSectionsX * chunk::kSectionBlocks,
       regionPos.y * world::kRegionSectionsY * chunk::kSectionBlocks,
       regionPos.z * world::kRegionSectionsZ * chunk::kSectionBlocks);
  }
  builder->setTerrainRegion(*region);
  region->addSection(raw);
  column[static_cast<std::size_t>(sectionY)] = std::move(builder);
  if(hasBlocks) {
   compilePipeline_->enqueueDirtyChunk(raw);
  }
 }
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  chunk::ChunkBuilder* raw = column[static_cast<std::size_t>(sectionY)].get();
  if(raw == nullptr) {
   continue;
  }
  if(sectionY > 0) {
   raw->neighbors[2] = column[static_cast<std::size_t>(sectionY - 1)].get();
  }
  if(sectionY + 1 < kChunkSectionCountY) {
   raw->neighbors[3] = column[static_cast<std::size_t>(sectionY + 1)].get();
  }
 }
 constexpr int horizontalFaces[] = {0, 1, 4, 5};
 for(int face : horizontalFaces) {
  const int otherFace = face ^ 1;
  const int neighborX = sectionX + kFaceDirX[face];
  const int neighborZ = sectionZ + kFaceDirZ[face];
  for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
   chunk::ChunkBuilder* self = column[static_cast<std::size_t>(sectionY)].get();
   chunk::ChunkBuilder* neighbor = sectionAt(neighborX, sectionY, neighborZ);
   if(self == nullptr || neighbor == nullptr) {
    continue;
   }
   self->neighbors[face] = neighbor;
   neighbor->neighbors[otherFace] = self;
  }
 }
}
void ChunkSectionSystem::removeColumn(int sectionX, int sectionZ) {
 const auto columnIt = columns_.find(world::ColumnPos{sectionX, sectionZ});
 if(columnIt == columns_.end()) {
  return;
 }
 const world::SectionColumn column = std::move(columnIt->second);
 columns_.erase(columnIt);
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  const std::shared_ptr<chunk::ChunkBuilder>& section = column[static_cast<std::size_t>(sectionY)];
  if(section == nullptr) {
   continue;
  }
  const world::SectionPos pos{sectionX, sectionY, sectionZ};
  for(int face = 0; face < 6; ++face) {
   chunk::ChunkBuilder* neighbor = section->neighbors[face];
   if(neighbor != nullptr) {
    neighbor->neighbors[face ^ 1] = nullptr;
    section->neighbors[face] = nullptr;
   }
  }
  for(net::minecraft::block::entity::BlockEntity* be : section->blockEntities_) {
   auto jt = std::find(scene_.blockEntities.begin(), scene_.blockEntities.end(), be);
   if(jt != scene_.blockEntities.end()) {
    scene_.blockEntities.erase(jt);
   }
  }
  const auto regionIt = regions_.find(world::regionOf(pos));
  compilePipeline_->releaseSection(*section);
  if(regionIt != regions_.end()) {
   regionIt->second->removeSection(section.get());
   if(regionIt->second->sections().empty()) {
    regions_.erase(regionIt);
   }
  }
 }
}
void ChunkSectionSystem::updateDebugCounts() {
 if(!scene_.options->debugHud) {
  debugCountCooldown_ = 0;
  return;
 }
 if(debugCountCooldown_ > 0) {
  --debugCountCooldown_;
  return;
 }
 debugCountCooldown_ = 9;
 chunkCount = 0;
 invisibleChunkCount = 0;
 compiledChunkCount = 0;
 emptyChunkCount = 0;
 builtChunkCount = 0;
 visibleBuiltChunkCount = 0;
 dirtyChunkCount = 0;
 inFlightChunkCount = 0;
 lightingPendingChunkCount = 0;
 forEachSection(regions_, [this](chunk::ChunkBuilder* chunk) {
  ++chunkCount;
  if(chunk->built) {
   ++builtChunkCount;
  }
  if(chunk->dirty) {
   ++dirtyChunkCount;
  }
  if(chunk->meshJobInFlight) {
   ++inFlightChunkCount;
  }
  if(!chunk->lightingReady) {
   ++lightingPendingChunkCount;
  }
  if(chunk->hasNoGeometry()) {
   ++emptyChunkCount;
  } else if(!chunk->visibleIn(frustumStamp_)) {
   ++invisibleChunkCount;
  } else {
   ++compiledChunkCount;
   if(chunk->built) {
    ++visibleBuiltChunkCount;
   }
  }
 });
}
void ChunkSectionSystem::clearSections() {
 compilePipeline_->cancelAll();
 for(auto& entry : columns_) {
  for(const std::shared_ptr<chunk::ChunkBuilder>& section : entry.second) {
   if(section != nullptr) {
    compilePipeline_->releaseSection(*section);
   }
  }
 }
 columns_.clear();
 regions_.clear();
 compilePipeline_->clearDirtyTracking();
 regularVisibleSections_.clear();
 scopedVisibleSections_.clear();
 scopedCull_ = false;
 scene_.blockEntities.clear();
 pendingColumns_.clear();
 pendingSet_.clear();
 pendingBorderRefresh_.clear();
 pendingLit_.clear();
}
void ChunkSectionSystem::drainPendingColumns() {
 if(scene_.world == nullptr) {
  return;
 }
 const net::minecraft::util::concurrent::FrameBudget budget =
     net::minecraft::util::concurrent::FrameBudget::fromSharedMs(2, 1);
 std::size_t inspected = 0;
 while(!pendingColumns_.empty() && budget.hasRemaining(static_cast<int>(inspected))) {
  const world::ColumnPos col = pendingColumns_.front();
  pendingColumns_.pop_front();
  pendingSet_.erase(col);
  ++inspected;
  if(sectionAt(col.x, 0, col.z) != nullptr) {
   continue;
  }
  // Drop, never requeue: every dataReady transition re-enqueues via chunkAvailable.
  if(scene_.world->getChunkSource() != nullptr && scene_.world->getChunkSource()->isChunkDataReady(col.x, col.z)) {
   createColumn(col.x, col.z);
  }
 }
}
void ChunkSectionSystem::reloadIfViewDistanceChanged() {
 const net::minecraft::client::option::GameOptions& opts = *scene_.options;
 if(opts.viewDistance != lastViewDistance) {
  if(scene_.reloadRequested) {
   scene_.reloadRequested();
  }
 }
}
void ChunkSectionSystem::cullChunks(Frustum* culler, bool updateGraph) {
 const FrameRenderCamera& renderCamera = core::cameraFrame();
 const net::minecraft::Vec3d camPos = WorldRenderer::sectionOrigin();
 if(updateGraph) {
  reloadIfViewDistanceChanged();
  drainPendingColumns();
 }
 std::vector<chunk::ChunkBuilder*>& visibleSections = currentVisibleSections();
 visibleSections.clear();
 const int stamp = ++frustumStamp_;
 if(renderCamera.shadowPass) {
  const ShadowCullingFrustum* shadowFrustum = renderCamera.shadowTerrainFrustum;
  for(auto& entry : regions_) {
   chunk::TerrainRegion& region = *entry.second;
   if(shadowFrustum != nullptr && !shadowFrustum->isVisible(regionCullingBox(region))) {
    continue;
   }
   for(chunk::ChunkBuilder* chunk : region.sections()) {
    // A built-but-empty section has nothing to raster and this pass never reads
    // back its visibility, so it can skip the plane test outright.
    if(chunk->hasNoGeometry()) {
     continue;
    }
    if(shadowFrustum == nullptr || shadowFrustum->isVisible(chunk->cullingBounds())) {
     visibleSections.push_back(chunk);
    }
   }
  }
  return;
 }
 if(culler == nullptr || !scene_.settings->frustumCulling) {
  forEachSection(regions_, [&visibleSections, stamp](chunk::ChunkBuilder* chunk) {
   chunk->visibleStamp = stamp;
   if(!chunk->hasNoGeometry()) {
    visibleSections.push_back(chunk);
   }
  });
 } else if(!scene_.settings->occlusionCulling || !applyOcclusionCulling(*culler, camPos, stamp)) {
  world::cullByFrustum(regions_, *culler, stamp, visibleSections);
 }
 updateDebugCounts();
}
bool ChunkSectionSystem::applyOcclusionCulling(const Frustum& culler,
                                              const net::minecraft::Vec3d& camPos,
                                              int stamp) {
 const int startX = MathHelper::floor(camPos.x) >> 4;
 const int startY = std::clamp(MathHelper::floor(camPos.y) >> 4, 0, kChunkSectionCountY - 1);
 const int startZ = MathHelper::floor(camPos.z) >> 4;
 chunk::ChunkBuilder* start = sectionAt(startX, startY, startZ);
 if(start == nullptr) {
  return false;
 }
 world::cullByOcclusionWalk(start, culler, stamp, occlusionQueue_, currentVisibleSections());
 return true;
}
std::string ChunkSectionSystem::getChunkDebugInfo() const {
 return "Chunks: vis " + std::to_string(compiledChunkCount) + "/" + std::to_string(chunkCount) +
        ", built " + std::to_string(visibleBuiltChunkCount) + "/" + std::to_string(builtChunkCount) +
        ", dirty " + std::to_string(dirtyChunkCount) + ", flight " + std::to_string(inFlightChunkCount) +
        ", light " + std::to_string(lightingPendingChunkCount) + ", cull " +
        std::to_string(invisibleChunkCount) + ", empty " + std::to_string(emptyChunkCount) + ", calls " +
        std::to_string(chunk::ChunkBuilder::frameDrawCalls);
}
void ChunkSectionSystem::markDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 const int startX = MathHelper::floorDiv(minX, chunk::kSectionBlocks);
 const int startY = MathHelper::floorDiv(minY, chunk::kSectionBlocks);
 const int startZ = MathHelper::floorDiv(minZ, chunk::kSectionBlocks);
 const int endX = MathHelper::floorDiv(maxX, chunk::kSectionBlocks);
 const int endY = MathHelper::floorDiv(maxY, chunk::kSectionBlocks);
 const int endZ = MathHelper::floorDiv(maxZ, chunk::kSectionBlocks);
 for(int chunkX = startX; chunkX <= endX; ++chunkX) {
  for(int chunkZ = startZ; chunkZ <= endZ; ++chunkZ) {
   for(int chunkY = startY; chunkY <= endY; ++chunkY) {
    chunk::ChunkBuilder* builder = sectionAt(chunkX, chunkY, chunkZ);
    if(builder == nullptr) {
     continue;
    }
    builder->invalidate();
    compilePipeline_->enqueueDirtyChunk(builder);
   }
  }
 }
}
void ChunkSectionSystem::blockUpdate(int x, int y, int z) {
 markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}
void ChunkSectionSystem::chunkAvailable(int chunkX, int chunkZ) {
 if(scene_.world == nullptr || scene_.world->getChunkSource() == nullptr ||
    !scene_.world->getChunkSource()->isChunkDataReady(chunkX, chunkZ)) {
  return;
 }
 enqueueColumn(chunkX, chunkZ);
 const world::ColumnPos key{chunkX, chunkZ};
 pendingLit_.insert(key);
 pendingBorderRefresh_.insert(key);
}
void ChunkSectionSystem::chunkUnloaded(int chunkX, int chunkZ) {
 // Must erase: a stale key makes enqueueColumn a no-op when the chunk reloads.
 const world::ColumnPos key{chunkX, chunkZ};
 pendingSet_.erase(key);
 pendingLit_.erase(key);
 pendingBorderRefresh_.erase(key);
 removeColumn(chunkX, chunkZ);
}
void ChunkSectionSystem::lightColumn(world::SectionColumn& column) {
 for(const std::shared_ptr<chunk::ChunkBuilder>& section : column) {
  if(section == nullptr) {
   continue;
  }
  section->lightingReady = true;
  if(section->dirty && !section->meshJobInFlight) {
   compilePipeline_->enqueueDirtyChunk(section.get());
  }
 }
}
void ChunkSectionSystem::markChunkColumnLit(int chunkX, int chunkZ) {
 // A column enters pendingLit_ once, in chunkAvailable, and leaves it the first time its
 // lighting lands; anything dirtied after that is re-enqueued by setBlocksDirty instead.
 if(pendingLit_.erase(world::ColumnPos{chunkX, chunkZ}) == 0) {
  return;
 }
 const auto it = columns_.find(world::ColumnPos{chunkX, chunkZ});
 if(it != columns_.end()) {
  lightColumn(it->second);
 }
}
void ChunkSectionSystem::markAllChunksLit() {
 if(pendingLit_.empty()) {
  return;
 }
 const std::unordered_set<world::ColumnPos, world::ColumnPosHash> pending = std::move(pendingLit_);
 pendingLit_.clear();
 for(const world::ColumnPos& column : pending) {
  const auto it = columns_.find(column);
  if(it != columns_.end()) {
   lightColumn(it->second);
  }
 }
}
void ChunkSectionSystem::drainBorderRefresh() {
 if(pendingBorderRefresh_.empty()) {
  return;
 }
 static constexpr int kNeighborX[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
 static constexpr int kNeighborZ[8] = {0, 0, -1, 1, -1, 1, -1, 1};
 for(auto columnIt = pendingBorderRefresh_.begin(); columnIt != pendingBorderRefresh_.end();) {
  const world::ColumnPos column = *columnIt;
  if(pendingLit_.contains(column)) {
   ++columnIt;
   continue;
  }
  for(int dir = 0; dir < 8; ++dir) {
   const int neighborX = column.x + kNeighborX[dir];
   const int neighborZ = column.z + kNeighborZ[dir];
   for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
   chunk::ChunkBuilder* section = sectionAt(neighborX, sectionY, neighborZ);
    if(section == nullptr || (!section->built && !section->meshJobInFlight)) {
     continue;
    }
    Chunk* sourceChunk = scene_.world->getChunkSource()->getChunkIfLoaded(neighborX, neighborZ);
    if(sourceChunk != nullptr && !sourceChunk->sectionHasBlocks(sectionY) && section->hasNoGeometry()) {
     continue;
    }
    section->invalidate();
    compilePipeline_->enqueueDirtyChunk(section);
   }
  }
  columnIt = pendingBorderRefresh_.erase(columnIt);
 }
}
void ChunkSectionSystem::setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 markDirty(minX - 1, minY - 1, minZ - 1, maxX + 1, maxY + 1, maxZ + 1);
}
void ChunkSectionSystem::notifyAmbientDarknessChanged() {
 for(auto& entry : columns_) {
  for(const std::shared_ptr<chunk::ChunkBuilder>& section : entry.second) {
   if(section == nullptr || !section->hasSkyLight || section->dirty) {
    continue;
   }
   section->invalidate();
   compilePipeline_->enqueueDirtyChunk(section.get());
  }
 }
}
void ChunkSectionSystem::updateBlockEntity(int x,
                                           int y,
                                           int z,
                                           net::minecraft::block::entity::BlockEntity* blockEntity) {
 markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}
namespace world {
void cullByFrustum(const RegionMap& regions,
                   const Frustum& culler,
                   int stamp,
                   std::vector<chunk::ChunkBuilder*>& out) {
 for(const auto& entry : regions) {
  chunk::TerrainRegion& region = *entry.second;
  if(!culler.isVisible(regionCullingBox(region))) {
   continue;
  }
  for(chunk::ChunkBuilder* chunk : region.sections()) {
   if(chunk->updateFrustum(culler, stamp) && !chunk->hasNoGeometry()) {
    out.push_back(chunk);
   }
  }
 }
}
void cullByOcclusionWalk(chunk::ChunkBuilder* start,
                         const Frustum& culler,
                         int stamp,
                         std::vector<OcclusionQueueEntry>& queue,
                         std::vector<chunk::ChunkBuilder*>& out) {
 queue.clear();
 // The camera's own section seeds the walk unconditionally: the eye is inside it,
 // so a plane test on it decides nothing and failing it would cull the world.
 (void)start->occlusion.enter(stamp, -1);
 start->visibleStamp = stamp;
 queue.push_back({start, -1});
 for(std::size_t head = 0; head < queue.size(); ++head) {
  const OcclusionQueueEntry entry = queue[head];
  chunk::ChunkBuilder* node = entry.section;
  if(entry.entryFace < 0 && !node->hasNoGeometry()) {
   out.push_back(node);
  }
  for(int face = 0; face < 6; ++face) {
   if(!node->occlusion.claimExit(stamp, entry.entryFace, face, node->built)) {
    continue;
   }
   chunk::ChunkBuilder* neighbor = node->neighbors[face];
   const int neighborEntryFace = face ^ 1;
   if(neighbor == nullptr || !neighbor->occlusion.enter(stamp, neighborEntryFace)) {
    continue;
   }
   const bool alreadyVisible = neighbor->visibleIn(stamp);
   if(alreadyVisible || neighbor->updateFrustum(culler, stamp)) {
    if(!alreadyVisible && !neighbor->hasNoGeometry()) {
     out.push_back(neighbor);
    }
    queue.push_back({neighbor, neighborEntryFace});
   }
  }
 }
}
} // namespace world
} // namespace net::minecraft::client::render
