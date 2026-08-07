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
bool boxTouchesSphere(const net::minecraft::Box& box,
                      double cx,
                      double cy,
                      double cz,
                      double radiusSq) {
 double dx = 0.0, dy = 0.0, dz = 0.0;
 if(cx < box.minX)
  dx = box.minX - cx;
 else if(cx > box.maxX)
  dx = cx - box.maxX;
 if(cy < box.minY)
  dy = box.minY - cy;
 else if(cy > box.maxY)
  dy = cy - box.maxY;
 if(cz < box.minZ)
  dz = box.minZ - cz;
 else if(cz > box.maxZ)
  dz = cz - box.maxZ;
 return dx * dx + dy * dy + dz * dz <= radiusSq;
}
template <class Fn>
void forEachSection(std::unordered_map<world::SectionPos, std::vector<chunk::ChunkBuilder*>, world::SectionPosHash>& regions,
                    Fn&& fn) {
 for(auto& entry : regions) {
  for(chunk::ChunkBuilder* chunk : entry.second) {
   fn(chunk);
  }
 }
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
int ChunkSectionSystem::renderRadiusChunks() const noexcept {
 return scene_.settings->renderDistance.chunks();
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
  builder->inFrustum = true;
  builder->invalidate();
  chunk::ChunkBuilder* raw = builder.get();
  sections_.emplace(pos, std::move(builder));
  regions_[world::regionOf(pos)].push_back(raw);
  compilePipeline_->enqueueDirtyChunk(raw);
 }
}
void ChunkSectionSystem::removeColumn(int sectionX, int sectionZ) {
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  const world::SectionPos pos{sectionX, sectionY, sectionZ};
  auto it = sections_.find(pos);
  if(it == sections_.end()) {
   continue;
  }
  std::shared_ptr<chunk::ChunkBuilder> section = std::move(it->second);
  sections_.erase(it);
  for(net::minecraft::block::entity::BlockEntity* be : section->blockEntities_) {
   auto jt = std::find(scene_.blockEntities.begin(), scene_.blockEntities.end(), be);
   if(jt != scene_.blockEntities.end()) {
    scene_.blockEntities.erase(jt);
   }
  }
  const auto regionIt = regions_.find(world::regionOf(pos));
  if(regionIt != regions_.end()) {
   std::vector<chunk::ChunkBuilder*>& bucket = regionIt->second;
   bucket.erase(std::remove(bucket.begin(), bucket.end(), section.get()), bucket.end());
   if(bucket.empty()) {
    regions_.erase(regionIt);
   }
  }
  compilePipeline_->releaseSection(*section);
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
  const int nodeX = node->x >> 4;
  const int nodeY = node->y >> 4;
  const int nodeZ = node->z >> 4;
  for(int face = 0; face < 6; ++face) {
   const int nextY = nodeY + kFaceDirY[face];
   if(nextY < 0 || nextY >= kChunkSectionCountY) {
    continue;
   }
   chunk::ChunkBuilder* neighbor = sectionAt(nodeX + kFaceDirX[face], nextY, nodeZ + kFaceDirZ[face]);
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
  return;
 }
 chunkCount = 0;
 invisibleChunkCount = 0;
 compiledChunkCount = 0;
 emptyChunkCount = 0;
 forEachSection(regions_, [this](chunk::ChunkBuilder* chunk) {
  ++chunkCount;
  if(chunk->hasNoGeometry()) {
   ++emptyChunkCount;
  } else if(!chunk->inFrustum) {
   ++invisibleChunkCount;
  } else {
   ++compiledChunkCount;
  }
 });
}
void ChunkSectionSystem::pushCullState() {
 savedVisibleSections_.swap(visibleSections_);
 cullStateSaved_ = true;
}
void ChunkSectionSystem::popCullState() {
 if(!cullStateSaved_) {
  return;
 }
 cullStateSaved_ = false;
 savedVisibleSections_.swap(visibleSections_);
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
 visibleSections_.clear();
 savedVisibleSections_.clear();
 cullStateSaved_ = false;
 scene_.blockEntities.clear();
 pendingColumns_.clear();
 pendingSet_.clear();
 pendingBorderRefresh_.clear();
 pendingLit_.clear();
 centerSectionX_ = std::numeric_limits<int>::min();
 centerSectionZ_ = std::numeric_limits<int>::min();
}
void ChunkSectionSystem::updateSectionFrontier() {
  const auto& frameCam = core::cameraFrame();
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
 const int radius = renderRadiusChunks();
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
 if(scene_.world == nullptr || centerSectionX_ == std::numeric_limits<int>::min()) {
  return;
 }
 const int radius = renderRadiusChunks();
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
  if(scene_.world->getChunkSource() != nullptr && scene_.world->getChunkSource()->isChunkDataReady(col.x, col.z)) {
   createColumn(col.x, col.z);
  } else {
   enqueueColumn(col.x, col.z);
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
void ChunkSectionSystem::cullChunks(Frustum* culler, bool updateFrontier) {
  const FrameRenderCamera& renderCamera = core::cameraFrame();
 const double bypassBlocks = std::max(0.0f, renderCamera.frustumBypassDistance);
 const double bypassSq = bypassBlocks * bypassBlocks;
 const net::minecraft::Vec3d camPos = WorldRenderer::sectionOrigin();
 if(updateFrontier) {
  reloadIfViewDistanceChanged();
  updateSectionFrontier();
  drainPendingColumns();
  rebuildSectionOrder(camPos);
 }
 if(renderCamera.shadowPass) {
  const ShadowCullingFrustum* shadowFrustum = renderCamera.shadowTerrainFrustum;
  visibleSections_.clear();
  forEachSection(regions_, [&](chunk::ChunkBuilder* chunk) {
   chunk->inFrustum =
       boxTouchesSphere(chunk->cullingBox, camPos.x, camPos.y, camPos.z, bypassSq) ||
       (shadowFrustum != nullptr && shadowFrustum->isVisible(chunk->cullingBox));
   if(chunk->inFrustum) {
    visibleSections_.push_back(chunk);
   }
  });
  updateDebugCounts();
  return;
 }
 if(culler == nullptr || !scene_.settings->frustumCulling) {
  visibleSections_.clear();
  forEachSection(regions_, [this](chunk::ChunkBuilder* chunk) {
   chunk->inFrustum = true;
   visibleSections_.push_back(chunk);
  });
  updateDebugCounts();
  return;
 }
 if(!scene_.settings->occlusionCulling) {
  visibleSections_.clear();
  forEachSection(regions_, [&](chunk::ChunkBuilder* chunk) {
   chunk->updateFrustum(*culler);
   if(boxTouchesSphere(chunk->cullingBox, camPos.x, camPos.y, camPos.z, bypassSq)) {
    chunk->inFrustum = true;
   }
   if(chunk->inFrustum) {
    visibleSections_.push_back(chunk);
   }
  });
  updateDebugCounts();
  return;
 }
 applyOcclusionCulling(culler, camPos, bypassSq);
 updateDebugCounts();
}
void ChunkSectionSystem::applyOcclusionCulling(Frustum* culler, const net::minecraft::Vec3d& camPos, double bypassSq) {
 visibleSections_.clear();
 const int startX = MathHelper::floor(camPos.x) >> 4;
 const int startY = std::clamp(MathHelper::floor(camPos.y) >> 4, 0, kChunkSectionCountY - 1);
 const int startZ = MathHelper::floor(camPos.z) >> 4;
 chunk::ChunkBuilder* start = sectionAt(startX, startY, startZ);
 if(start == nullptr) {
  forEachSection(regions_, [&](chunk::ChunkBuilder* chunk) {
   chunk->updateFrustum(*culler);
   if(boxTouchesSphere(chunk->cullingBox, camPos.x, camPos.y, camPos.z, bypassSq)) {
    chunk->inFrustum = true;
   }
   if(chunk->inFrustum) {
    visibleSections_.push_back(chunk);
   }
  });
  return;
 }
 const int stamp = ++occlusionStamp_;
 occlusionQueue_.clear();
 start->occlusion.enter(stamp, -1);
 occlusionQueue_.push_back(start);
 for(std::size_t head = 0; head < occlusionQueue_.size(); ++head) {
  chunk::ChunkBuilder* node = occlusionQueue_[head];
  node->updateFrustum(*culler);
  if(boxTouchesSphere(node->cullingBox, camPos.x, camPos.y, camPos.z, bypassSq)) {
   node->inFrustum = true;
  }
  if(node->inFrustum) {
   visibleSections_.push_back(node);
  }
  const int nodeX = node->x >> 4;
  const int nodeY = node->y >> 4;
  const int nodeZ = node->z >> 4;
  for(int face = 0; face < 6; ++face) {
   if(!node->occlusion.connects(face, node->built)) {
    continue;
   }
   const int nextY = nodeY + kFaceDirY[face];
   if(nextY < 0 || nextY >= kChunkSectionCountY) {
    continue;
   }
   chunk::ChunkBuilder* neighbor = sectionAt(nodeX + kFaceDirX[face], nextY, nodeZ + kFaceDirZ[face]);
   if(neighbor == nullptr || neighbor->occlusion.visitedIn(stamp)) {
    continue;
   }
   neighbor->occlusion.enter(stamp, face ^ 1);
   occlusionQueue_.push_back(neighbor);
  }
 }
 forEachSection(regions_, [&](chunk::ChunkBuilder* chunk) {
  if(chunk->occlusion.visitedIn(stamp)) {
   return;
  }
  if(boxTouchesSphere(chunk->cullingBox, camPos.x, camPos.y, camPos.z, bypassSq)) {
   chunk->inFrustum = true;
   visibleSections_.push_back(chunk);
  } else {
   chunk->inFrustum = false;
  }
 });
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
   if(std::abs(chunkX - centerSectionX_) <= renderRadiusChunks() &&
      std::abs(chunkZ - centerSectionZ_) <= renderRadiusChunks()) {
    enqueueColumn(chunkX, chunkZ);
   }
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
 if(centerSectionX_ == std::numeric_limits<int>::min()) {
  return;
 }
 if(scene_.world == nullptr || scene_.world->getChunkSource() == nullptr ||
    !scene_.world->getChunkSource()->isChunkLoaded(chunkX, chunkZ)) {
  return;
 }
 if(std::abs(chunkX - centerSectionX_) > renderRadiusChunks() + 1 ||
    std::abs(chunkZ - centerSectionZ_) > renderRadiusChunks() + 1) {
  return;
 }
 enqueueColumn(chunkX, chunkZ);
 pendingLit_.insert(world::SectionPos{chunkX, 0, chunkZ});
 pendingBorderRefresh_.insert(world::SectionPos{chunkX, 0, chunkZ});
}
void ChunkSectionSystem::chunkUnloaded(int chunkX, int chunkZ) {
 pendingLit_.erase(world::SectionPos{chunkX, 0, chunkZ});
 pendingBorderRefresh_.erase(world::SectionPos{chunkX, 0, chunkZ});
 removeColumn(chunkX, chunkZ);
}
void ChunkSectionSystem::markChunkColumnLit(int chunkX, int chunkZ) {
 pendingLit_.erase(world::SectionPos{chunkX, 0, chunkZ});
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  chunk::ChunkBuilder* section = sectionAt(chunkX, sectionY, chunkZ);
  if(section != nullptr && section->dirty && !section->built) {
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
   if(section != nullptr && section->dirty && !section->built) {
    compilePipeline_->enqueueDirtyChunk(section);
   }
  }
 }
}
void ChunkSectionSystem::drainBorderRefresh() {
 if(pendingBorderRefresh_.empty()) {
  return;
 }
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
    compilePipeline_->enqueueDirtyChunk(section);
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
