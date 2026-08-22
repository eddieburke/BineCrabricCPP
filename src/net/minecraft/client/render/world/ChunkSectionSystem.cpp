#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/debug/VTuneTrace.hpp"
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
void forEachSection(
    std::unordered_map<world::SectionPos, std::unique_ptr<chunk::TerrainRegion>, world::SectionPosHash>& regions,
    Fn&& fn) {
 for(auto& entry : regions) {
  for(chunk::ChunkBuilder* chunk : entry.second->sections()) {
   fn(chunk);
  }
 }
}
// One plane test per region rejects up to kRegionSectionsX*Y*Z sections at once.
// The padding matches ChunkBuilder's cullingBox padding so this box conservatively
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
const net::minecraft::entity::LivingEntity* ChunkSectionSystem::frontierCamera() const {
 if(scene_.camera != nullptr) {
  if(const auto* living = dynamic_cast<const net::minecraft::entity::LivingEntity*>(scene_.camera)) {
   return living;
  }
 }
 return scene_.client != nullptr ? scene_.client->camera : nullptr;
}
chunk::ChunkBuilder* ChunkSectionSystem::sectionAt(int sectionX, int sectionY, int sectionZ) {
 const auto it = sections_.find(world::SectionPos{sectionX, sectionY, sectionZ});
 return it == sections_.end() ? nullptr : it->second.get();
}
void ChunkSectionSystem::enqueueColumn(int sectionX, int sectionZ) {
 if(sectionAt(sectionX, 0, sectionZ) != nullptr) {
  return;
 }
 const world::SectionPos key{sectionX, 0, sectionZ};
 if(pendingSet_.insert(key).second) {
  pendingColumns_.push_back(key);
 }
}
void ChunkSectionSystem::createColumn(int sectionX, int sectionZ) {
 if(scene_.world == nullptr || scene_.world->getChunkSource() == nullptr ||
    !scene_.world->getChunkSource()->isChunkLoaded(sectionX, sectionZ)) {
  return;
 }
 const bool lightingReady = !pendingLit_.contains(world::SectionPos{sectionX, 0, sectionZ});
 chunk::ChunkBuilder* column[kChunkSectionCountY] = {};
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  const world::SectionPos pos{sectionX, sectionY, sectionZ};
  if(sections_.contains(pos)) {
   continue;
  }
  auto builder = std::make_shared<chunk::ChunkBuilder>(scene_.world,
                                                       scene_.blockEntities,
                                                       sectionX * chunk::kSectionBlocks,
                                                       sectionY * chunk::kSectionBlocks,
                                                       sectionZ * chunk::kSectionBlocks,
                                                       chunk::kSectionBlocks);
  builder->lightingReady = lightingReady;
  builder->invalidate();
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
  column[sectionY] = raw;
  sections_.emplace(pos, std::move(builder));
  compilePipeline_->enqueueDirtyChunk(raw);
 }
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  chunk::ChunkBuilder* raw = column[sectionY];
  if(raw == nullptr) {
   continue;
  }
  if(sectionY > 0) {
   raw->neighbors[2] = column[sectionY - 1];
  }
  if(sectionY + 1 < kChunkSectionCountY) {
   raw->neighbors[3] = column[sectionY + 1];
  }
 }
 constexpr int horizontalFaces[] = {0, 1, 4, 5};
 for(int face : horizontalFaces) {
  const int otherFace = face ^ 1;
  const int neighborX = sectionX + kFaceDirX[face];
  const int neighborZ = sectionZ + kFaceDirZ[face];
  for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
   chunk::ChunkBuilder* self = column[sectionY];
   chunk::ChunkBuilder* neighbor = sectionAt(neighborX, sectionY, neighborZ);
   if(self == nullptr || neighbor == nullptr) {
    continue;
   }
   self->neighbors[face] = neighbor;
   neighbor->neighbors[otherFace] = self;
  }
 }
 sectionsChanged_ = true;
}
void ChunkSectionSystem::removeColumn(int sectionX, int sectionZ) {
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  const world::SectionPos pos{sectionX, sectionY, sectionZ};
  auto it = sections_.find(pos);
  if(it == sections_.end()) {
   continue;
  }
  std::shared_ptr<chunk::ChunkBuilder> section = std::move(it->second);
  for(int face = 0; face < 6; ++face) {
   chunk::ChunkBuilder* neighbor = section->neighbors[face];
   if(neighbor != nullptr) {
    neighbor->neighbors[face ^ 1] = nullptr;
    section->neighbors[face] = nullptr;
   }
  }
  sections_.erase(it);
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
void ChunkSectionSystem::rebuildSectionOrder(const net::minecraft::Vec3d& camPos) {
 ++meshOrderStamp_;
 sectionsByPriority_.clear();
 const int startX = centerSectionX_;
 const int startZ = centerSectionZ_;
 const int startY = std::clamp(MathHelper::floor(camPos.y) >> 4, 0, kChunkSectionCountY - 1);
 chunk::ChunkBuilder* start = sectionAt(startX, startY, startZ);
 if(start == nullptr) {
  return;
 }
 start->meshOrderStamp = meshOrderStamp_;
 start->meshPriority = 0;
 sectionsByPriority_.push_back(start);
 for(std::size_t head = 0; head < sectionsByPriority_.size(); ++head) {
  chunk::ChunkBuilder* node = sectionsByPriority_[head];
  for(int face = 0; face < 6; ++face) {
   chunk::ChunkBuilder* neighbor = node->neighbors[face];
   if(neighbor == nullptr || neighbor->meshOrderStamp == meshOrderStamp_) {
    continue;
   }
   neighbor->meshOrderStamp = meshOrderStamp_;
   neighbor->meshPriority = node->meshPriority + 1;
   sectionsByPriority_.push_back(neighbor);
  }
 }
 forEachSection(regions_, [this](chunk::ChunkBuilder* chunk) {
  if(chunk->meshOrderStamp != meshOrderStamp_) {
   chunk->meshOrderStamp = meshOrderStamp_;
   chunk->meshPriority = static_cast<int>(sectionsByPriority_.size());
   sectionsByPriority_.push_back(chunk);
  }
 });
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
 forEachSection(regions_, [this](chunk::ChunkBuilder* chunk) {
  ++chunkCount;
  if(chunk->hasNoGeometry()) {
   ++emptyChunkCount;
  } else if(!chunk->visibleIn(frustumStamp_)) {
   ++invisibleChunkCount;
  } else {
   ++compiledChunkCount;
  }
 });
}
void ChunkSectionSystem::clearSections() {
 compilePipeline_->cancelAll();
 for(auto& entry : sections_) {
  if(entry.second != nullptr) {
   compilePipeline_->releaseSection(*entry.second);
  }
 }
 sections_.clear();
 regions_.clear();
 compilePipeline_->clearDirtyTracking();
 sectionsByPriority_.clear();
 regularVisibleSections_.clear();
 scopedVisibleSections_.clear();
 scopedCull_ = false;
 scene_.blockEntities.clear();
 pendingColumns_.clear();
 pendingSet_.clear();
 pendingBorderRefresh_.clear();
 pendingLit_.clear();
 centerSectionX_ = std::numeric_limits<int>::min();
 centerSectionZ_ = std::numeric_limits<int>::min();
 sectionsChanged_ = true;
}
bool ChunkSectionSystem::updateCameraSection() {
 const auto& frameCam = core::cameraFrame();
 double camX = frameCam.x;
 double camZ = frameCam.z;
 if(camX == 0.0 && camZ == 0.0) {
  const net::minecraft::entity::LivingEntity* camera = frontierCamera();
  if(camera == nullptr) {
   return false;
  }
  camX = camera->x;
  camZ = camera->z;
 }
 const int camSectionX = MathHelper::floor(camX) >> 4;
 const int camSectionZ = MathHelper::floor(camZ) >> 4;
 if(camSectionX == centerSectionX_ && camSectionZ == centerSectionZ_) {
  return false;
 }
 centerSectionX_ = camSectionX;
 centerSectionZ_ = camSectionZ;
 return true;
}
void ChunkSectionSystem::updateProfileMetrics(bool shadowPass) {
 if(shadowPass) {
  VT_TRACE_COUNTER("ShadowSectionsVisible", visibleSections().size());
  return;
 }
 VT_TRACE_COUNTER("ChunkColumnsPending", pendingColumns_.size());
 VT_TRACE_COUNTER("LightingColumnsPending", pendingLit_.size());
 VT_TRACE_COUNTER("BorderRefreshColumnsPending", pendingBorderRefresh_.size());
 VT_TRACE_COUNTER("ChunkSectionsResident", sections_.size());
 VT_TRACE_COUNTER("ChunkSectionsVisible", visibleSections().size());
}
void ChunkSectionSystem::drainPendingColumns() {
 if(scene_.world == nullptr) {
  return;
 }
 const net::minecraft::util::concurrent::FrameBudget budget =
     net::minecraft::util::concurrent::FrameBudget::fromSharedMs(2, 1);
 std::size_t inspected = 0;
 while(!pendingColumns_.empty() && budget.hasRemaining(static_cast<int>(inspected))) {
  const world::SectionPos col = pendingColumns_.front();
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
 const option::RenderSettings& resolved = *scene_.settings;
 if(opts.viewDistance != lastViewDistance || resolved.renderScale != lastRenderScale) {
  if(scene_.reloadRequested) {
   scene_.reloadRequested();
  }
 }
}
void ChunkSectionSystem::cullChunks(Frustum* culler, bool updateGraph) {
 const FrameRenderCamera& renderCamera = core::cameraFrame();
 VT_TRACE_EVENT(renderCamera.shadowPass ? "terrain/shadow_cull" : "terrain/view_cull");
 const net::minecraft::Vec3d camPos = WorldRenderer::sectionOrigin();
 if(updateGraph) {
  reloadIfViewDistanceChanged();
  const bool cameraSectionChanged = updateCameraSection();
  drainPendingColumns();
  if(cameraSectionChanged || sectionsChanged_) {
   sectionsChanged_ = false;
   rebuildSectionOrder(camPos);
  }
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
    if(shadowFrustum == nullptr || shadowFrustum->isVisible(chunk->cullingBox)) {
     visibleSections.push_back(chunk);
    }
   }
  }
  updateProfileMetrics(true);
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
  for(auto& entry : regions_) {
   chunk::TerrainRegion& region = *entry.second;
   if(!culler->isVisible(regionCullingBox(region))) {
    continue;
   }
   for(chunk::ChunkBuilder* chunk : region.sections()) {
    if(chunk->updateFrustum(*culler, stamp) && !chunk->hasNoGeometry()) {
     visibleSections.push_back(chunk);
    }
   }
  }
 }
 updateDebugCounts();
 updateProfileMetrics(false);
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
 std::vector<chunk::ChunkBuilder*>& visibleSections = currentVisibleSections();
 occlusionQueue_.clear();
 // The camera's own section seeds the walk unconditionally: the eye is inside it,
 // so a plane test on it decides nothing and failing it would cull the world.
 start->occlusion.enter(stamp, -1);
 start->visibleStamp = stamp;
 occlusionQueue_.push_back(start);
 for(std::size_t head = 0; head < occlusionQueue_.size(); ++head) {
  chunk::ChunkBuilder* node = occlusionQueue_[head];
  if(!node->hasNoGeometry()) {
   visibleSections.push_back(node);
  }
  for(int face = 0; face < 6; ++face) {
   if(!node->occlusion.connects(face, node->built)) {
    continue;
   }
   chunk::ChunkBuilder* neighbor = node->neighbors[face];
   if(neighbor == nullptr || neighbor->occlusion.visitedIn(stamp)) {
    continue;
   }
   // Mark visited even when culled: the plane test does not depend on which face
   // we arrived through, so reaching it again from another face cannot change the
   // answer and would only re-test it.
   neighbor->occlusion.enter(stamp, face ^ 1);
   if(neighbor->updateFrustum(culler, stamp)) {
    occlusionQueue_.push_back(neighbor);
   }
  }
 }
 return true;
}
std::string ChunkSectionSystem::getChunkDebugInfo() const {
 return "Chunks drawn: " + std::to_string(compiledChunkCount) + " of " + std::to_string(chunkCount) +
        ". Frustum-culled: " + std::to_string(invisibleChunkCount) + ", Empty: " +
        std::to_string(emptyChunkCount) + ", Draw calls: " +
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
 pendingLit_.insert(world::SectionPos{chunkX, 0, chunkZ});
 pendingBorderRefresh_.insert(world::SectionPos{chunkX, 0, chunkZ});
}
void ChunkSectionSystem::chunkUnloaded(int chunkX, int chunkZ) {
 // Must erase: a stale key makes enqueueColumn a no-op when the chunk reloads.
 pendingSet_.erase(world::SectionPos{chunkX, 0, chunkZ});
 pendingLit_.erase(world::SectionPos{chunkX, 0, chunkZ});
 pendingBorderRefresh_.erase(world::SectionPos{chunkX, 0, chunkZ});
 removeColumn(chunkX, chunkZ);
}
void ChunkSectionSystem::markChunkColumnLit(int chunkX, int chunkZ) {
 pendingLit_.erase(world::SectionPos{chunkX, 0, chunkZ});
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  chunk::ChunkBuilder* section = sectionAt(chunkX, sectionY, chunkZ);
  if(section != nullptr) {
   section->lightingReady = true;
  }
  if(section != nullptr && section->dirty && !section->meshJobInFlight) {
   compilePipeline_->enqueueDirtyChunk(section);
  }
 }
}
void ChunkSectionSystem::markAllChunksLit() {
 if(pendingLit_.empty()) {
  return;
 }
 std::vector<world::SectionPos> columns;
 columns.reserve(pendingLit_.size());
 for(const world::SectionPos& column : pendingLit_) {
  columns.push_back(column);
 }
 pendingLit_.clear();
 for(const world::SectionPos& column : columns) {
  for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
   chunk::ChunkBuilder* section = sectionAt(column.x, sectionY, column.z);
   if(section != nullptr) {
    section->lightingReady = true;
   }
   if(section != nullptr && section->dirty && !section->meshJobInFlight) {
    compilePipeline_->enqueueDirtyChunk(section);
   }
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
  const world::SectionPos column = *columnIt;
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
 for(auto& entry : sections_) {
  chunk::ChunkBuilder& chunk = *entry.second;
  if(!chunk.hasSkyLight || chunk.dirty) {
   continue;
  }
  chunk.invalidate();
  compilePipeline_->enqueueDirtyChunk(&chunk);
 }
}
void ChunkSectionSystem::updateBlockEntity(int x,
                                           int y,
                                           int z,
                                           net::minecraft::block::entity::BlockEntity* blockEntity) {
 markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}
} // namespace net::minecraft::client::render
