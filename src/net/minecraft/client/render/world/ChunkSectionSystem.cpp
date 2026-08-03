#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
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
constexpr int kChunkSectionSize = 16;
constexpr int kChunkSectionCountY = 8;
} // namespace
const net::minecraft::entity::LivingEntity* ChunkSectionSystem::frontierCamera() const {
 if(facade_.cameraEntity_ != nullptr) {
  if(const auto* living = dynamic_cast<const net::minecraft::entity::LivingEntity*>(facade_.cameraEntity_)) {
   return living;
  }
 }
 return facade_.client != nullptr ? facade_.client->camera : nullptr;
}
chunk::ChunkBuilder* ChunkSectionSystem::sectionAt(int sectionX, int sectionY, int sectionZ) {
 const auto it = sections_.find(world::SectionPos{sectionX, sectionY, sectionZ});
 return it == sections_.end() ? nullptr : it->second.get();
}
int ChunkSectionSystem::ringOf(int sectionX, int sectionZ) const noexcept {
 const int dx = std::abs(sectionX - centerSectionX_);
 const int dz = std::abs(sectionZ - centerSectionZ_);
 int ring = dx > dz ? dx : dz;
 if(ring < 0) {
  ring = 0;
 }
 if(ring > renderRadiusChunks_) {
  ring = renderRadiusChunks_;
 }
 return ring;
}
int ChunkSectionSystem::ringOf(const chunk::ChunkBuilder& chunk) const noexcept {
 return ringOf(chunk.x >> 4, chunk.z >> 4);
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
 if(facade_.world == nullptr || facade_.world->getChunkSource() == nullptr ||
    !facade_.world->getChunkSource()->isChunkLoaded(sectionX, sectionZ)) {
  return;
 }
  const int ring = ringOf(sectionX, sectionZ);
  if(ring >= static_cast<int>(drawRings_.size())) {
   drawRings_.resize(static_cast<std::size_t>(ring) + 1);
  }
  for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  const world::SectionPos pos{sectionX, sectionY, sectionZ};
  if(sections_.contains(pos)) {
   continue;
  }
  auto builder = std::make_unique<chunk::ChunkBuilder>(facade_.world,
                                                       facade_.globalBlockEntities,
                                                       sectionX * kChunkSectionSize,
                                                       sectionY * kChunkSectionSize,
                                                       sectionZ * kChunkSectionSize,
                                                       kChunkSectionSize,
                                                       &facade_.compilePipeline_.regionManager());
   builder->inFrustum = true;
  builder->invalidate();
  chunk::ChunkBuilder* raw = builder.get();
  sections_.emplace(pos, std::move(builder));
  sectionList_.push_back(raw);
  raw->drawRing = ring;
  drawRings_[static_cast<std::size_t>(ring)].insert(raw);
  facade_.compilePipeline_.enqueueDirtyChunk(raw);
 }
}
void ChunkSectionSystem::removeColumn(int sectionX, int sectionZ) {
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  const world::SectionPos pos{sectionX, sectionY, sectionZ};
  auto it = sections_.find(pos);
  if(it == sections_.end()) {
   continue;
  }
  std::unique_ptr<chunk::ChunkBuilder> section = std::move(it->second);
  sections_.erase(it);
  for(net::minecraft::block::entity::BlockEntity* be : section->blockEntities_) {
   auto jt = std::find(facade_.globalBlockEntities.begin(), facade_.globalBlockEntities.end(), be);
   if(jt != facade_.globalBlockEntities.end()) {
    facade_.globalBlockEntities.erase(jt);
   }
  }
  sectionList_.erase(std::remove(sectionList_.begin(), sectionList_.end(), section.get()), sectionList_.end());
  if(section->drawRing >= 0 && section->drawRing < static_cast<int>(drawRings_.size())) {
   drawRings_[static_cast<std::size_t>(section->drawRing)].erase(section.get());
  }
  facade_.compilePipeline_.retireOrFreeSection(std::move(section));
 }
}
void ChunkSectionSystem::rebuildDrawRings() {
 drawRings_.assign(static_cast<std::size_t>(renderRadiusChunks_) + 1, {});
 for(chunk::ChunkBuilder* chunk : sectionList_) {
  drawRings_[static_cast<std::size_t>(ringOf(*chunk))].insert(chunk);
 }
 rebuildVisibleDrawRings();
}
void ChunkSectionSystem::rebuildVisibleDrawRings() {
 visibleDrawRings_.resize(drawRings_.size());
 const bool collectDebugCounts = facade_.activeOptions().debugHud;
 if(collectDebugCounts) {
  chunkCount = static_cast<int>(sectionList_.size());
  invisibleChunkCount = 0;
  compiledChunkCount = 0;
  emptyChunkCount = 0;
 }
 for(std::size_t ring = 0; ring < drawRings_.size(); ++ring) {
  const auto& src = drawRings_[ring];
  std::vector<chunk::ChunkBuilder*>& dst = visibleDrawRings_[ring];
  dst.clear();
  dst.reserve(src.size());
  for(chunk::ChunkBuilder* chunk : src) {
   if(collectDebugCounts && chunk != nullptr) {
    if(chunk->hasNoGeometry()) {
     ++emptyChunkCount;
    } else if(!chunk->inFrustum) {
     ++invisibleChunkCount;
    } else {
     ++compiledChunkCount;
    }
   }
   if(chunk != nullptr && chunk->inFrustum) {
    dst.push_back(chunk);
   }
  }
 }
}
void ChunkSectionSystem::pushCullState() {
 // Swap, never copy: visibleDrawRings_ picks up the previous scratch buffers
 // (whose inner capacity the nested pass immediately reuses via
 // rebuildVisibleDrawRings), and the player's rings park in the scratch slot.
 savedVisibleDrawRings_.swap(visibleDrawRings_);
 cullStateSaved_ = true;
}
void ChunkSectionSystem::popCullState() {
 if(!cullStateSaved_) {
  return;
 }
 cullStateSaved_ = false;
 savedVisibleDrawRings_.swap(visibleDrawRings_);
 // No inFrustum backup/restore needed: the nested (shadow) pass runs before the
 // main pass's cullChunks, which rebuilds inFrustum from scratch afterwards.
}
void ChunkSectionSystem::clearSections() {
 facade_.compilePipeline_.cancelAll();
 // Non-blocking cancelAll: a worker may still hold the last shared_ptr of an
 // in-flight job whose builder lives in sections_ (R3). Those sections must
 // outlive the job; retire them and let sweepRetiring reap them once the job
 // drains and ~ChunkMeshJob clears meshJobInFlight on the main thread.
 for(auto& entry : sections_) {
  if(entry.second != nullptr) {
   facade_.compilePipeline_.retireOrFreeSection(std::move(entry.second));
  }
 }
 sections_.clear();
 sectionList_.clear();
 facade_.compilePipeline_.clearDirtyTracking();
 drawRings_.clear();
 visibleDrawRings_.clear();
 // Dangling ChunkBuilder* would otherwise survive in the nested-pass scratch.
 savedVisibleDrawRings_.clear();
 cullStateSaved_ = false;
  facade_.globalBlockEntities.clear();
    pendingColumns_.clear();
    pendingSet_.clear();
    pendingBorderRefresh_.clear();
    pendingLit_.clear();
  // Retired sections still reference the shared region pool until reaped; only
  // clear it once nothing is in flight.
 facade_.compilePipeline_.clearRegionPool();
 centerSectionX_ = std::numeric_limits<int>::min();
 centerSectionZ_ = std::numeric_limits<int>::min();
}
void ChunkSectionSystem::updateSectionFrontier() {
 const auto& frameCam = RenderCameraState::instance().frame();
 double camX = frameCam.x;
 double camZ = frameCam.z;
 if(camX == 0.0 && camZ == 0.0) {
  const net::minecraft::entity::LivingEntity* camera = frontierCamera();
  if(camera == nullptr) {
   return;
  }
  camX = camera->x;
  camZ = camera->z;
 }
 const int camSectionX = MathHelper::floor(camX) >> 4;
 const int camSectionZ = MathHelper::floor(camZ) >> 4;
 if(camSectionX == centerSectionX_ && camSectionZ == centerSectionZ_) {
  return;
 }
 const int oldCenterX = centerSectionX_;
 const int oldCenterZ = centerSectionZ_;
 centerSectionX_ = camSectionX;
 centerSectionZ_ = camSectionZ;
 const int radius = renderRadiusChunks_;
 const bool teleported = oldCenterX == std::numeric_limits<int>::min() ||
                         std::abs(camSectionX - oldCenterX) > radius || std::abs(camSectionZ - oldCenterZ) > radius;
 if(teleported) {
  clearSections();
  centerSectionX_ = camSectionX;
  centerSectionZ_ = camSectionZ;
  for(int sx = camSectionX - radius; sx <= camSectionX + radius; ++sx) {
   for(int sz = camSectionZ - radius; sz <= camSectionZ + radius; ++sz) {
    enqueueColumn(sx, sz);
   }
  }
  return;
 }
 const int oldMinX = oldCenterX - radius;
 const int oldMaxX = oldCenterX + radius;
 const int oldMinZ = oldCenterZ - radius;
 const int oldMaxZ = oldCenterZ + radius;
 const int newMinX = camSectionX - radius;
 const int newMaxX = camSectionX + radius;
 const int newMinZ = camSectionZ - radius;
 const int newMaxZ = camSectionZ + radius;
 const auto visitRect = [](int minX, int maxX, int minZ, int maxZ, const auto& visit) {
  if(minX > maxX || minZ > maxZ) {
   return;
  }
  for(int sx = minX; sx <= maxX; ++sx) {
   for(int sz = minZ; sz <= maxZ; ++sz) {
    visit(sx, sz);
   }
  }
 };
 const auto remove = [this](int sx, int sz) { removeColumn(sx, sz); };
 const auto enqueue = [this](int sx, int sz) { enqueueColumn(sx, sz); };
 const auto visitOutside = [&visitRect](int baseMinX,
                                        int baseMaxX,
                                        int baseMinZ,
                                        int baseMaxZ,
                                        int innerMinX,
                                        int innerMaxX,
                                        int innerMinZ,
                                        int innerMaxZ,
                                        const auto& visit) {
  visitRect(baseMinX, std::min(baseMaxX, innerMinX - 1), baseMinZ, baseMaxZ, visit);
  visitRect(std::max(baseMinX, innerMaxX + 1), baseMaxX, baseMinZ, baseMaxZ, visit);
  const int overlapMinX = std::max(baseMinX, innerMinX);
  const int overlapMaxX = std::min(baseMaxX, innerMaxX);
  visitRect(overlapMinX, overlapMaxX, baseMinZ, std::min(baseMaxZ, innerMinZ - 1), visit);
  visitRect(overlapMinX, overlapMaxX, std::max(baseMinZ, innerMaxZ + 1), baseMaxZ, visit);
 };
 visitOutside(oldMinX, oldMaxX, oldMinZ, oldMaxZ, newMinX, newMaxX, newMinZ, newMaxZ, remove);
 visitOutside(newMinX, newMaxX, newMinZ, newMaxZ, oldMinX, oldMaxX, oldMinZ, oldMaxZ, enqueue);
}
void ChunkSectionSystem::drainPendingColumns() {
 if(facade_.world == nullptr || centerSectionX_ == std::numeric_limits<int>::min()) {
  return;
 }
 const int radius = renderRadiusChunks_;
 const net::minecraft::util::concurrent::FrameBudget budget =
     net::minecraft::util::concurrent::FrameBudget::fromSharedMs(2, 1);
 std::size_t inspected = 0;
 while(!pendingColumns_.empty() && budget.hasRemaining(static_cast<int>(inspected))) {
  const world::SectionPos col = pendingColumns_.front();
  pendingColumns_.pop_front();
  pendingSet_.erase(col);
  ++inspected;
  if(std::abs(col.x - centerSectionX_) > radius || std::abs(col.z - centerSectionZ_) > radius) {
   continue;
  }
  if(sectionAt(col.x, 0, col.z) != nullptr) {
   continue;
  }
  if(facade_.world->getChunkSource() != nullptr && facade_.world->getChunkSource()->isChunkDataReady(col.x, col.z)) {
   createColumn(col.x, col.z);
  } else {
   enqueueColumn(col.x, col.z);
  }
 }
}
void ChunkSectionSystem::reloadIfViewDistanceChanged() {
 const net::minecraft::client::option::GameOptions& opts = facade_.activeOptions();
 const option::RenderSettings& resolved = facade_.frameSettings();
 if(opts.viewDistance != lastViewDistance || resolved.renderScale != lastRenderScale) {
  facade_.reload();
 }
}
void ChunkSectionSystem::cullChunks(FrustumCuller* culler, float /*tickDelta*/, bool updateFrontier) {
 const FrameRenderCamera& renderCamera = RenderCameraState::instance().frame();
 const double nearFrustumBypassBlocks = std::max(0.0f, renderCamera.frustumBypassDistance);
 const double nearFrustumBypassDistanceSq = nearFrustumBypassBlocks * nearFrustumBypassBlocks;
 const double camX = core::drawCameraStateValid() ? static_cast<double>(core::drawCameraPosition()[0])
                                                   : renderCamera.eyeX;
 const double camY = core::drawCameraStateValid() ? static_cast<double>(core::drawCameraPosition()[1])
                                                   : renderCamera.eyeY;
 const double camZ = core::drawCameraStateValid() ? static_cast<double>(core::drawCameraPosition()[2])
                                                   : renderCamera.eyeZ;
 if(updateFrontier) {
  reloadIfViewDistanceChanged();
  updateSectionFrontier();
  drainPendingColumns();
 }
 // The shadow pass has its own frustum (Java ShadowRenderer.createShadowFrustum): the
 // player's view volume extruded toward the light, so casters outside the player's
 // frustum that still shadow it survive. Nothing about the camera's own frustum, the
 // near bypass or occlusion culling applies here.
 if(renderCamera.shadowPass) {
  const ShadowCullingFrustum* shadowFrustum = renderCamera.shadowTerrainFrustum;
  for(chunk::ChunkBuilder* chunk : sectionList_) {
   chunk->inFrustum = shadowFrustum == nullptr || shadowFrustum->isVisible(chunk->cullingBox);
  }
  rebuildVisibleDrawRings();
  return;
 }
  if(culler == nullptr || !facade_.frameSettings().frustumCulling) {
  for(chunk::ChunkBuilder* chunk : sectionList_) {
   chunk->inFrustum = true;
  }
  rebuildVisibleDrawRings();
  return;
 }
 for(chunk::ChunkBuilder* chunkPtr : sectionList_) {
  chunk::ChunkBuilder& chunk = *chunkPtr;
  chunk.updateFrustum(*culler);
  // Sections hugging the camera are kept regardless of the frustum so nothing pops in
  // at the near plane. The radius is spherical, like the occlusion bypass below: a
  // horizontal-only test left camY unused and made this an infinite vertical cylinder,
  // which kept every section directly above and below the player in view.
  const double dx = camX - static_cast<double>(chunk.centerX);
  const double dy = camY - static_cast<double>(chunk.centerY);
  const double dz = camZ - static_cast<double>(chunk.centerZ);
  if(dx * dx + dy * dy + dz * dz <= nearFrustumBypassDistanceSq) {
   chunk.inFrustum = true;
  }
 }
  applyOcclusionCulling();
  rebuildVisibleDrawRings();
}
void ChunkSectionSystem::applyOcclusionCulling() {
 if(!facade_.frameSettings().occlusionCulling) {
  return;
 }
 constexpr double kNearOcclusionBypassSq = 48.0 * 48.0;
 const auto& frameCam = RenderCameraState::instance().frame();
 double camX = frameCam.x;
 double camY = frameCam.y;
 double camZ = frameCam.z;
 if(camX == 0.0 && camY == 0.0 && camZ == 0.0) {
  if(facade_.cameraEntity_ != nullptr) {
   camX = facade_.cameraEntity_->x;
   camY = facade_.cameraEntity_->y;
   camZ = facade_.cameraEntity_->z;
  } else {
   return;
  }
 }
 const int startX = MathHelper::floor(camX) >> 4;
 int startY = MathHelper::floor(camY) >> 4;
 const int startZ = MathHelper::floor(camZ) >> 4;
 if(startY < 0) {
  startY = 0;
 }
 if(startY >= kChunkSectionCountY) {
  startY = kChunkSectionCountY - 1;
 }
 chunk::ChunkBuilder* start = sectionAt(startX, startY, startZ);
 if(start == nullptr) {
  return;
 }
 const int stamp = ++occlusionStamp_;
 occlusionQueue_.clear();
 start->occStamp = stamp;
 start->occEntryFace = -1;
 occlusionQueue_.push_back(start);
 static constexpr int kDirX[6] = {-1, 1, 0, 0, 0, 0};
 static constexpr int kDirY[6] = {0, 0, -1, 1, 0, 0};
 static constexpr int kDirZ[6] = {0, 0, 0, 0, -1, 1};
 for(std::size_t head = 0; head < occlusionQueue_.size(); ++head) {
  chunk::ChunkBuilder* node = occlusionQueue_[head];
  const int nodeX = node->x >> 4;
  const int nodeY = node->y >> 4;
  const int nodeZ = node->z >> 4;
  for(int face = 0; face < 6; ++face) {
   if(node->occEntryFace >= 0 && node->built &&
      (node->visBits & (1ULL << (node->occEntryFace * 6 + face))) == 0) {
    continue;
   }
   const int nextY = nodeY + kDirY[face];
   if(nextY < 0 || nextY >= kChunkSectionCountY) {
    continue;
   }
   chunk::ChunkBuilder* neighbor = sectionAt(nodeX + kDirX[face], nextY, nodeZ + kDirZ[face]);
   if(neighbor == nullptr || neighbor->occStamp == stamp) {
    continue;
   }
   neighbor->occStamp = stamp;
   neighbor->occEntryFace = face ^ 1;
   occlusionQueue_.push_back(neighbor);
  }
 }
 for(chunk::ChunkBuilder* chunkPtr : sectionList_) {
  if(chunkPtr->occStamp == stamp || !chunkPtr->inFrustum) {
   continue;
  }
  const double dx = camX - static_cast<double>(chunkPtr->centerX);
  const double dy = camY - static_cast<double>(chunkPtr->centerY);
  const double dz = camZ - static_cast<double>(chunkPtr->centerZ);
  if(dx * dx + dy * dy + dz * dz <= kNearOcclusionBypassSq) {
   continue;
  }
  chunkPtr->inFrustum = false;
 }
}
std::string ChunkSectionSystem::getChunkDebugInfo() const {
 return "Chunks drawn: " + std::to_string(compiledChunkCount) + " of " + std::to_string(chunkCount) +
        ". Frustum-culled: " + std::to_string(invisibleChunkCount) + ", Empty: " +
        std::to_string(emptyChunkCount) + ", Draw calls: " +
        std::to_string(chunk::ChunkRegionBuffer::frameDrawCalls) + "/" +
        std::to_string(chunk::ChunkRegionBuffer::frameVisibleRanges);
}
void ChunkSectionSystem::markDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 const int startX = MathHelper::floorDiv(minX, kChunkSectionSize);
 const int startY = MathHelper::floorDiv(minY, kChunkSectionSize);
 const int startZ = MathHelper::floorDiv(minZ, kChunkSectionSize);
 const int endX = MathHelper::floorDiv(maxX, kChunkSectionSize);
 const int endY = MathHelper::floorDiv(maxY, kChunkSectionSize);
 const int endZ = MathHelper::floorDiv(maxZ, kChunkSectionSize);
 for(int chunkX = startX; chunkX <= endX; ++chunkX) {
  for(int chunkZ = startZ; chunkZ <= endZ; ++chunkZ) {
   if(std::abs(chunkX - centerSectionX_) <= renderRadiusChunks_ &&
      std::abs(chunkZ - centerSectionZ_) <= renderRadiusChunks_) {
    enqueueColumn(chunkX, chunkZ);
   }
   for(int chunkY = startY; chunkY <= endY; ++chunkY) {
    chunk::ChunkBuilder* builder = sectionAt(chunkX, chunkY, chunkZ);
    if(builder == nullptr) {
     continue;
    }
    builder->invalidate();
    facade_.compilePipeline_.enqueueDirtyChunk(builder);
   }
  }
 }
}
void ChunkSectionSystem::blockUpdate(int x, int y, int z) {
 markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}
void ChunkSectionSystem::chunkAvailable(int chunkX, int chunkZ) {
 if(centerSectionX_ == std::numeric_limits<int>::min()) {
  return;
 }
 // Defensive: never gate/enqueue a column whose chunk is not actually loaded
 // (an eviction may have raced the notification).
 if(facade_.world == nullptr || facade_.world->getChunkSource() == nullptr ||
    !facade_.world->getChunkSource()->isChunkLoaded(chunkX, chunkZ)) {
  return;
 }
 // Allow one extra ring: a column just outside the render radius still shares a
 // border with one just inside it, and that inside column's faces were meshed
 // against the missing data.
   if(std::abs(chunkX - centerSectionX_) > renderRadiusChunks_ + 1 ||
      std::abs(chunkZ - centerSectionZ_) > renderRadiusChunks_ + 1) {
   return;
  }
   enqueueColumn(chunkX, chunkZ);
   pendingLit_.insert(world::SectionPos{chunkX, 0, chunkZ});
   pendingBorderRefresh_.insert(world::SectionPos{chunkX, 0, chunkZ});
}
// The chunk cache evicted a column. Drop the mirroring sections (plus any stale
// gate / border-refresh bookkeeping) so the renderer never keeps meshes for data
// that is gone, and a later reload rebuilds the column from scratch instead of
// reusing stale state.
void ChunkSectionSystem::chunkUnloaded(int chunkX, int chunkZ) {
 pendingLit_.erase(world::SectionPos{chunkX, 0, chunkZ});
 pendingBorderRefresh_.erase(world::SectionPos{chunkX, 0, chunkZ});
 removeColumn(chunkX, chunkZ);
}
// P-LITGATE: the column's lighting drained (World::doLightingUpdates). Release
// its gate and re-enqueue the unbuilt sections whose first mesh was held.
void ChunkSectionSystem::markChunkColumnLit(int chunkX, int chunkZ) {
 pendingLit_.erase(world::SectionPos{chunkX, 0, chunkZ});
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  chunk::ChunkBuilder* section = sectionAt(chunkX, sectionY, chunkZ);
  if(section != nullptr && section->dirty && !section->built) {
   facade_.compilePipeline_.enqueueDirtyChunk(section);
  }
 }
}
// P-LITGATE: the lighting engine went fully idle — no remaining boxes could
// produce a region for a held column, so every gate is released (non-optional
// completion; a column with no pending boxes is never held forever).
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
   if(section != nullptr && section->dirty && !section->built) {
    facade_.compilePipeline_.enqueueDirtyChunk(section);
   }
  }
 }
}
void ChunkSectionSystem::drainBorderRefresh() {
 if(pendingBorderRefresh_.empty()) {
  return;
 }
 // Only the four orthogonal neighbours share a face with the arriving column;
 // diagonals are reached through those neighbours' own one-block shell.
 //
 // A section is skipped only when it has never been built AND has no job in
 // flight: createColumn already marked it dirty and queued it, so it will mesh
 // against the new data anyway. A section with a job IN FLIGHT must still be
 // invalidated even though it reads as dirty — its snapshot was captured before
 // this chunk arrived, and without bumping version compileChunks would accept
 // that stale mesh and clear the dirty flag, leaving a stale border.
 static constexpr int kNeighborX[4] = {-1, 1, 0, 0};
 static constexpr int kNeighborZ[4] = {0, 0, -1, 1};
 for(const world::SectionPos& column : pendingBorderRefresh_) {
  for(int dir = 0; dir < 4; ++dir) {
   const int neighborX = column.x + kNeighborX[dir];
   const int neighborZ = column.z + kNeighborZ[dir];
   for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
    chunk::ChunkBuilder* section = sectionAt(neighborX, sectionY, neighborZ);
    if(section == nullptr || (!section->built && !section->meshJobInFlight)) {
     continue;
    }
    section->invalidate();
    facade_.compilePipeline_.enqueueDirtyChunk(section);
   }
  }
 }
 pendingBorderRefresh_.clear();
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
  facade_.compilePipeline_.enqueueDirtyChunk(&chunk);
 }
}
void ChunkSectionSystem::updateBlockEntity(int x,
                                           int y,
                                           int z,
                                           net::minecraft::block::entity::BlockEntity* /*blockEntity*/) {
 markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}
} // namespace net::minecraft::client::render
