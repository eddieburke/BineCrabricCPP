#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/particle/ParticleRegistry.hpp"
#include "net/minecraft/client/particle/PickupParticle.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/pipeline/Manager.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/mod/lua/LuaModEntity.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/util/concurrent/FrameBudget.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
namespace net::minecraft::client::render {
namespace {
constexpr int kChunkSectionSize = 16;
constexpr int kChunkSectionCountY = 8;
constexpr int kBaselineRadius = 13;
struct ModChunkMeshScope {
 render::RenderPassScope passScope_;
 explicit ModChunkMeshScope(const RenderType& rt) : passScope_(rt) {
 }
 ModChunkMeshScope(const ModChunkMeshScope&) = delete;
 ModChunkMeshScope& operator=(const ModChunkMeshScope&) = delete;
};
struct WorldOverlayScope {
 render::RenderPassScope passScope_;
 explicit WorldOverlayScope(const RenderType& rt) : passScope_(rt) {
  core::enableDepthTest();
  core::depthMask(false);
  core::enableBlend();
  core::blendAlpha();
 }
 ~WorldOverlayScope() {
  core::disableBlend();
 }
 WorldOverlayScope(const WorldOverlayScope&) = delete;
 WorldOverlayScope& operator=(const WorldOverlayScope&) = delete;
};
struct WorldCrackOverlayScope {
 render::RenderPassScope passScope_;
 explicit WorldCrackOverlayScope(const RenderType& rt) : passScope_(rt) {
  core::enableDepthTest();
  core::depthMask(false);
  core::enableCull();
  core::cullBackFaces();
  core::blendAlpha();
  core::polygonOffset(-3.0f, -3.0f);
  core::enablePolygonOffset();
 }
 ~WorldCrackOverlayScope() {
  core::disablePolygonOffset();
  core::disableBlend();
 }
 WorldCrackOverlayScope(const WorldCrackOverlayScope&) = delete;
 WorldCrackOverlayScope& operator=(const WorldCrackOverlayScope&) = delete;
};
struct BlockOutlineScope {
 RenderPassScope pass;
 core::RenderStageScope stage;
 BlockOutlineScope() : pass(RenderType::lines()), stage(core::RenderStage::Outline) {
  core::enableBlend();
  core::blendAlpha();
  core::enableDepthTest();
  core::depthMask(false);
 }
 ~BlockOutlineScope() {
  core::enableDepthTest();
  core::depthMask(true);
  core::disableBlend();
 }
 BlockOutlineScope(const BlockOutlineScope&) = delete;
 BlockOutlineScope& operator=(const BlockOutlineScope&) = delete;
};
} // namespace
WorldRenderer::WorldRenderer(net::minecraft::client::Minecraft* minecraftIn,
                             net::minecraft::client::texture::TextureManager* textureManagerIn)
    : client(minecraftIn), textureManager(textureManagerIn) {
 blockRenderManager.ctx.textureManager = textureManager;
 if(client != nullptr) {
  options_ = &client->options;
 }
 settings_ = option::renderSettings(activeOptions());
}
net::minecraft::client::option::GameOptions& WorldRenderer::activeOptions() const {
 if(options_ != nullptr) {
  return *options_;
 }
 if(client != nullptr) {
  return const_cast<net::minecraft::client::option::GameOptions&>(client->options);
 }
 return const_cast<WorldRenderer*>(this)->defaultOptions_;
}
const net::minecraft::LivingEntity* WorldRenderer::frontierCamera() const {
 if(cameraEntity_ != nullptr) {
  if(const auto* living = dynamic_cast<const net::minecraft::LivingEntity*>(cameraEntity_)) {
   return living;
  }
 }
 return client != nullptr ? client->camera : nullptr;
}
chunk::ChunkBuilder* WorldRenderer::sectionAt(int sectionX, int sectionY, int sectionZ) {
 const auto it = sections_.find(world::SectionPos{sectionX, sectionY, sectionZ});
 return it == sections_.end() ? nullptr : it->second.get();
}
int WorldRenderer::ringOf(int sectionX, int sectionZ) const noexcept {
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
int WorldRenderer::ringOf(const chunk::ChunkBuilder& chunk) const noexcept {
 return ringOf(chunk.x >> 4, chunk.z >> 4);
}
void WorldRenderer::enqueueDirtyChunk(chunk::ChunkBuilder* chunk) {
 if(chunk == nullptr || chunk->meshJobInFlight) {
  return;
 }
 // P-LITGATE: hold the FIRST mesh of a freshly-created column until its column
 // is marked lit (markChunkColumnLit re-enqueues it). Re-meshes of already-built
 // sections are never held — the gate only delays the first build.
 if(!lightingGate_.lit(chunk->x >> 4, chunk->z >> 4) && !chunk->built) {
  return;
 }
 noteNearDirty(chunk);
 dirtyChunks_.insert(chunk);
}
void WorldRenderer::noteNearDirty(chunk::ChunkBuilder* chunk) {
 constexpr float kNearDirtyDistSq = 32.0f * 32.0f;
 constexpr std::size_t kNearDirtyCap = 64;
 if(nearDirtyChunks_.size() >= kNearDirtyCap) {
  return;
 }
 const net::minecraft::Entity* camera =
     cameraEntity_ != nullptr ? cameraEntity_ : (client != nullptr ? client->camera : nullptr);
 if(camera == nullptr || chunk->squaredDistanceTo(camera->x, camera->y, camera->z) > kNearDirtyDistSq) {
  return;
 }
 nearDirtyChunks_.insert(chunk);
}
void WorldRenderer::enqueueColumn(int sectionX, int sectionZ) {
 if(sectionAt(sectionX, 0, sectionZ) != nullptr) {
  return;
 }
 const world::SectionPos key{sectionX, 0, sectionZ};
 if(pendingSet_.insert(key).second) {
  pendingColumns_.push_back(key);
 }
}
void WorldRenderer::createColumn(int sectionX, int sectionZ) {
 if(world == nullptr || world->getChunkSource() == nullptr ||
    !world->getChunkSource()->isChunkLoaded(sectionX, sectionZ)) {
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
  auto builder = std::make_unique<chunk::ChunkBuilder>(world,
                                                       globalBlockEntities,
                                                       sectionX * kChunkSectionSize,
                                                       sectionY * kChunkSectionSize,
                                                       sectionZ * kChunkSectionSize,
                                                       kChunkSectionSize,
                                                       &regionManager_);
   builder->inFrustum = true;
  builder->invalidate();
  chunk::ChunkBuilder* raw = builder.get();
  sections_.emplace(pos, std::move(builder));
  sectionList_.push_back(raw);
  raw->drawRing = ring;
  drawRings_[static_cast<std::size_t>(ring)].insert(raw);
  enqueueDirtyChunk(raw);
 }
}
void WorldRenderer::removeColumn(int sectionX, int sectionZ) {
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  const world::SectionPos pos{sectionX, sectionY, sectionZ};
  auto it = sections_.find(pos);
  if(it == sections_.end()) {
   continue;
  }
  std::unique_ptr<chunk::ChunkBuilder> section = std::move(it->second);
  sections_.erase(it);
  for(net::minecraft::block::entity::BlockEntity* be : section->blockEntities_) {
   auto jt = std::find(globalBlockEntities.begin(), globalBlockEntities.end(), be);
   if(jt != globalBlockEntities.end()) {
    globalBlockEntities.erase(jt);
   }
  }
  sectionList_.erase(std::remove(sectionList_.begin(), sectionList_.end(), section.get()), sectionList_.end());
  if(section->drawRing >= 0 && section->drawRing < static_cast<int>(drawRings_.size())) {
   drawRings_[static_cast<std::size_t>(section->drawRing)].erase(section.get());
  }
  retireOrFreeSection(std::move(section));
 }
}
void WorldRenderer::retireOrFreeSection(std::unique_ptr<chunk::ChunkBuilder> section) {
 if(section == nullptr) {
  return;
 }
 dirtyChunks_.erase(section.get());
 nearDirtyChunks_.erase(section.get());
 if(section->meshJobInFlight) {
  section->retired = true;
  retiring_.push_back(std::move(section));
  return;
 }
 section->freeRegionSlots();
}
void WorldRenderer::sweepRetiring() {
 for(auto it = retiring_.begin(); it != retiring_.end();) {
  if((*it)->meshJobInFlight) {
   ++it;
   continue;
  }
  (*it)->freeRegionSlots();
  it = retiring_.erase(it);
 }
}
void WorldRenderer::rebuildDrawRings() {
 drawRings_.assign(static_cast<std::size_t>(renderRadiusChunks_) + 1, {});
 for(chunk::ChunkBuilder* chunk : sectionList_) {
  drawRings_[static_cast<std::size_t>(ringOf(*chunk))].insert(chunk);
 }
 rebuildVisibleDrawRings();
}
void WorldRenderer::rebuildVisibleDrawRings() {
 visibleDrawRings_.resize(drawRings_.size());
 const bool collectDebugCounts = activeOptions().debugHud;
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
void WorldRenderer::pushCullState() {
 // Swap, never copy: visibleDrawRings_ picks up the previous scratch buffers
 // (whose inner capacity the nested pass immediately reuses via
 // rebuildVisibleDrawRings), and the player's rings park in the scratch slot.
 savedVisibleDrawRings_.swap(visibleDrawRings_);
 cullStateSaved_ = true;
}
void WorldRenderer::popCullState() {
 if(!cullStateSaved_) {
  return;
 }
 cullStateSaved_ = false;
 savedVisibleDrawRings_.swap(visibleDrawRings_);
 // No inFrustum backup/restore needed: the nested (shadow) pass runs before the
 // main pass's cullChunks, which rebuilds inFrustum from scratch afterwards.
}
void WorldRenderer::clearSections() {
 meshScheduler_.cancelAll();
 pendingMeshUploads_.clear();
 // Non-blocking cancelAll: a worker may still hold the last shared_ptr of an
 // in-flight job whose builder lives in sections_ (R3). Those sections must
 // outlive the job; retire them and let sweepRetiring reap them once the job
 // drains and ~ChunkMeshJob clears meshJobInFlight on the main thread.
 for(auto& entry : sections_) {
  if(entry.second != nullptr) {
   retireOrFreeSection(std::move(entry.second));
  }
 }
 sections_.clear();
 sectionList_.clear();
 dirtyChunks_.clear();
 nearDirtyChunks_.clear();
 drawRings_.clear();
 visibleDrawRings_.clear();
 // Dangling ChunkBuilder* would otherwise survive in the nested-pass scratch.
 savedVisibleDrawRings_.clear();
 cullStateSaved_ = false;
 globalBlockEntities.clear();
  pendingColumns_.clear();
  pendingSet_.clear();
  pendingBorderRefresh_.clear();
  lightingGate_.releaseAll();
 // Retired sections still reference the shared region pool until reaped; only
 // clear it once nothing is in flight.
 if(retiring_.empty()) {
  regionManager_.clear();
 }
 centerSectionX_ = std::numeric_limits<int>::min();
 centerSectionZ_ = std::numeric_limits<int>::min();
}
void WorldRenderer::updateSectionFrontier() {
 const auto& frameCam = RenderCameraState::instance().frame();
 double camX = frameCam.x;
 double camZ = frameCam.z;
 if(camX == 0.0 && camZ == 0.0) {
  const net::minecraft::LivingEntity* camera = frontierCamera();
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
void WorldRenderer::drainPendingColumns() {
 if(world == nullptr || centerSectionX_ == std::numeric_limits<int>::min()) {
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
  if(world->getChunkSource() != nullptr && world->getChunkSource()->isChunkDataReady(col.x, col.z)) {
   createColumn(col.x, col.z);
  } else {
   enqueueColumn(col.x, col.z);
  }
 }
}
void WorldRenderer::setWorld(net::minecraft::World* worldIn) {
 if(world != nullptr) {
  world->removeEventListener(this);
 }
 world = worldIn;
 blockRenderManager.setBlockView(worldIn);
 if(client != nullptr) {
  entity::EntityRenderDispatcher::instance().setWorld(worldIn);
 }
 if(world != nullptr) {
  world->addEventListener(this);
  reload();
 } else {
  clearSections();
  cameraEntity_ = nullptr;
 }
}
void WorldRenderer::reload() {
 clearSections();
 if(world == nullptr) {
  return;
 }
 // Resolve options + pack bake flags before sections rebuild. Stale
 // separateAo/oldLighting from the previous pack made Smooth Lighting look
 // broken until the slider forced another remesh.
 const PackDefinition* pack = nullptr;
 if(client != nullptr && client->gameRenderer != nullptr && client->gameRenderer->shaderPacks() != nullptr) {
  pack = client->gameRenderer->shaderPacks()->meshDefinition();
 }
 settings_ = option::renderSettings(activeOptions(), pack);
 net::minecraft::client::option::GameOptions& opts = activeOptions();
 const option::RenderSettings& resolved = settings_;
 if(Block::LEAVES != nullptr) {
  static_cast<net::minecraft::block::LeavesBlock*>(Block::LEAVES)->setFancyGraphics(resolved.fancyLeaves);
 }
 block::BlockRenderManager::fancyLeaves = resolved.fancyLeaves;
 blockRenderManager.snapshotGlobals();
 lastViewDistance = opts.viewDistance;
 lastRenderScale = resolved.renderScale;
  gl::GLCore::ensureLoaded();
  renderRadiusChunks_ = resolved.chunkRadius;
   // Pre-size the shared terrain VBO so the initial chunk stream does not repeatedly
   // grow the buffer and re-upload every accumulated range (a per-frame stall on
   // world load / teleport). Estimate ~384 vertices/section/layer at steady state,
   // capped so a giant view distance does not reserve gigabytes up front.
   if(renderRadiusChunks_ > 0) {
    const std::size_t columns = static_cast<std::size_t>(2 * renderRadiusChunks_ + 1);
    const std::size_t sections = columns * columns * static_cast<std::size_t>(kChunkSectionCountY);
    const std::size_t estimate = std::min<std::size_t>(sections * 384u, 16u * 1024u * 1024u);
    chunk::ChunkRegion& region = regionManager_.pool();
    for(auto& layer : region.layers) layer.reserve(estimate);
    net::minecraft::client::render::quad_index::ensure(estimate);
   }
  globalBlockEntities.clear();
  entityRenderCooldown = 2;
}
void WorldRenderer::reloadIfViewDistanceChanged() {
 const net::minecraft::client::option::GameOptions& opts = activeOptions();
 const option::RenderSettings& resolved = settings_;
 if(opts.viewDistance != lastViewDistance || resolved.renderScale != lastRenderScale) {
  reload();
 }
}
bool WorldRenderer::startMeshJob(chunk::ChunkBuilder* chunk,
                                 bool nearLane,
                                 int priority,
                                 const client::option::RenderSettings& resolvedOpts,
                                 bool fancyGraphics) {
 if(chunk == nullptr || chunk->meshJobInFlight || !chunk->dirty) {
  return false;
 }
 auto job = chunk::ChunkMeshJob::capture(*chunk, resolvedOpts, fancyGraphics);
 if(job == nullptr) {
  return false;
 }
 // Snapshot capture runs on the mesh worker while pins are held.
 chunk->meshJobInFlight = true;
 dirtyChunks_.erase(chunk);
 if(nearLane) {
  meshScheduler_.enqueueNear(std::move(job));
 } else {
  meshScheduler_.enqueue(std::move(job), priority);
 }
 return true;
}
int WorldRenderer::render(net::minecraft::LivingEntity& camera, int layer, double tickDelta, bool drawModMeshes) {
 if(sections_.empty()) {
  return 0;
 }
 cameraEntity_ = &camera;
 renderChunks(layer, tickDelta, drawModMeshes);
 return lastDrawnRegionCount_;
}
void WorldRenderer::renderLastChunks(int layer, double tickDelta) {
 // Java's GameRenderer draws translucent terrain twice when fancyGraphics is
 // on: once with colorMask off (depth-only prepass) via render(), then again
 // here with colorMask restored so blending reads back a depth buffer that
 // already matches, avoiding z-fighting between overlapping translucent
 // faces (water against glass, etc). Re-running the same layer draw covers
 // the VBO region path.
 renderChunks(layer, tickDelta, true, true);
}
int WorldRenderer::renderChunksVbo(
    int layer, double /*tickDelta*/, double interpX, double interpY, double interpZ, bool skipBuildDrawLists) {
 lastDrawnRegionCount_ = 0;
 chunk::ChunkRegion& pool = regionManager_.pool();
 chunk::ChunkRegionBuffer& buffer = pool.layers[static_cast<std::size_t>(layer)];
 // chunkOffset = sectionOrigin - active draw camera (player or shadow).
 double camX = interpX;
 double camY = interpY;
 double camZ = interpZ;
 if(core::drawCameraStateValid()) {
  const float* eye = core::drawCameraPosition();
  camX = static_cast<double>(eye[0]);
  camY = static_cast<double>(eye[1]);
  camZ = static_cast<double>(eye[2]);
 }
 if(!skipBuildDrawLists) {
  // Reset on the first layer of the player's own pass only. Nested passes
  // (sun shadow) set renderCameraEntity_, and letting
  // them reset would leave the F3 counter showing the shadow map's draw calls
  // instead of the frame's.
  if(layer == 0 && !renderCameraEntity_) {
   chunk::ChunkRegionBuffer::frameVisibleRanges = 0;
   chunk::ChunkRegionBuffer::frameDrawCalls = 0;
  }
  buffer.beginFrame();
  for(const std::vector<chunk::ChunkBuilder*>& ring : visibleDrawRings_) {
   for(chunk::ChunkBuilder* chunk : ring) {
    if(chunk == nullptr || chunk->region_ == nullptr ||
       chunk->renderLayerEmpty[static_cast<std::size_t>(layer)]) {
     continue;
    }
    const chunk::ChunkRegionBuffer::Slot& slot = chunk->regionSlots_[static_cast<std::size_t>(layer)];
    if(!slot.valid() || slot.count <= 0) {
     continue;
    }
    const float ox = static_cast<float>(static_cast<double>(chunk->x) - camX);
    const float oy = static_cast<float>(static_cast<double>(chunk->y) - camY);
    const float oz = static_cast<float>(static_cast<double>(chunk->z) - camZ);
    buffer.addVisible(slot, ox, oy, oz);
   }
  }
 }
 if(!buffer.hasVisible()) {
  return 0;
 }
 lastDrawnRegionCount_ = buffer.flush();
 return lastDrawnRegionCount_;
}
void WorldRenderer::renderChunks(int layer, double tickDelta, bool drawModMeshes, bool skipBuildDrawLists) {
 double interpX = 0.0;
 double interpY = 0.0;
 double interpZ = 0.0;
 cameraInterpPosition(tickDelta, interpX, interpY, interpZ);
 lastDrawnRegionCount_ = renderChunksVbo(layer, tickDelta, interpX, interpY, interpZ, skipBuildDrawLists);
 if(drawModMeshes) {
  lastDrawnRegionCount_ += renderModChunkMeshes(layer, interpX, interpY, interpZ);
 }
}
int WorldRenderer::renderModChunkMeshes(int layer, double interpX, double interpY, double interpZ) {
 if(textureManager == nullptr) {
  return 0;
 }
 const RenderType& renderType = layer == chunk::terrain_layer::Translucent
                                    ? RenderType::translucent()
                                    : layer == chunk::terrain_layer::Cutout ? RenderType::cutout()
                                                                           : RenderType::solid();
 const ModChunkMeshScope meshCaps(renderType);
 int drawn = 0;
 double camX = interpX;
 double camY = interpY;
 double camZ = interpZ;
 if(core::drawCameraStateValid()) {
  const float* eye = core::drawCameraPosition();
  camX = static_cast<double>(eye[0]);
  camY = static_cast<double>(eye[1]);
  camZ = static_cast<double>(eye[2]);
 }
 struct ModMeshDraw {
  int textureId;
  chunk::ChunkBuilder* chunk;
  const TessellatorMesh* mesh;
 };
 std::vector<ModMeshDraw> ringDraws;
 for(const std::vector<chunk::ChunkBuilder*>& ring : visibleDrawRings_) {
  ringDraws.clear();
  for(chunk::ChunkBuilder* chunk : ring) {
   if(chunk == nullptr || !chunk->inFrustum) {
    continue;
   }
   for(const chunk::ModChunkMesh& modMesh : chunk->modLayerMeshes_[static_cast<std::size_t>(layer)]) {
    if(modMesh.mesh.empty()) {
     continue;
    }
    ringDraws.push_back({modMesh.texture, chunk, &modMesh.mesh});
   }
  }
  if(ringDraws.empty()) {
   continue;
  }
  std::stable_sort(ringDraws.begin(), ringDraws.end(), [](const ModMeshDraw& a, const ModMeshDraw& b) {
   return a.textureId < b.textureId;
  });
  int boundTextureId = -1;
  int boundGlId = -1;
  bool boundValid = false;
  for(const ModMeshDraw& entry : ringDraws) {
   if(!boundValid || entry.textureId != boundTextureId) {
    boundTextureId = entry.textureId;
    boundGlId = net::minecraft::registry::TextureRegistry::resolveGlId(entry.textureId, *textureManager);
    boundValid = true;
    if(boundGlId >= 0) {
     textureManager->bindTexture(boundGlId);
    }
   }
   if(boundGlId < 0) {
    continue;
   }
   const float ox = static_cast<float>(static_cast<double>(entry.chunk->x) - camX);
   const float oy = static_cast<float>(static_cast<double>(entry.chunk->y) - camY);
   const float oz = static_cast<float>(static_cast<double>(entry.chunk->z) - camZ);
   core::setPendingTerrainDraw(ox, oy, oz);
   Tessellator::drawMesh(*entry.mesh);
   core::clearPendingTerrainDraw();
   ++drawn;
  }
 }
 return drawn;
}
bool WorldRenderer::compileChunks(net::minecraft::LivingEntity& /*camera*/, bool force) {
 drainBorderRefresh();
 const client::option::RenderSettings& resolvedOpts = settings_;
 const bool fancyGraphics = activeOptions().fancyGraphics;
 const float gridAreaScale = static_cast<float>(renderRadiusChunks_ * renderRadiusChunks_) /
                             static_cast<float>(kBaselineRadius * kBaselineRadius);
 const std::size_t workerCount = meshScheduler_.workerCount();
 const std::size_t backlog = dirtyChunks_.size() + pendingMeshUploads_.size() + meshScheduler_.pendingJobs();
 const bool loadingBacklog = backlog > 512u;
 const int minUploadsPerFrame = loadingBacklog ? std::clamp(static_cast<int>(workerCount * 2u), 4, 16)
                                               : std::clamp(static_cast<int>(std::ceil(2.0f * gridAreaScale)), 1, 6);
 const net::minecraft::util::concurrent::FrameBudget uploadBudget =
     net::minecraft::util::concurrent::FrameBudget::fromSharedMs(loadingBacklog ? 6 : 3, minUploadsPerFrame);
 // Near-camera edits get a dedicated slice (QD-21): they are uploaded before
 // the ring backlog so a block edit next to the player lands the frame its
 // mesh finishes, not when the distant ring's budget allows (HZ-31).
 const net::minecraft::util::concurrent::FrameBudget nearBudget =
     net::minecraft::util::concurrent::FrameBudget::fromSharedMs(2, minUploadsPerFrame);
 int uploadCount = 0;
 int nearUploadCount = 0;
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> deferredUploads;
 deferredUploads.reserve(pendingMeshUploads_.size() + meshScheduler_.pendingJobs());
 const auto processUpload = [&](std::shared_ptr<chunk::ChunkMeshJob> job, bool nearLane) {
  chunk::ChunkBuilder* builder = job->builder;
  if(builder == nullptr) {
   return;
  }
  if(builder->retired) {
   builder->meshJobInFlight = false;
   return;
  }
  const net::minecraft::util::concurrent::FrameBudget& budget = nearLane ? nearBudget : uploadBudget;
  const int budgetCount = nearLane ? nearUploadCount : uploadCount;
  if(!budget.hasRemaining(budgetCount)) {
   deferredUploads.push_back(std::move(job));
   return;
  }
  builder->meshJobInFlight = false;
  if(job->failed || job->version != builder->version) {
   builder->dirty = true;
   enqueueDirtyChunk(builder);
   return;
  }
  builder->uploadMesh(*job);
  builder->dirty = false;
  dirtyChunks_.erase(builder);
  if(nearLane) {
   ++nearUploadCount;
  } else {
   ++uploadCount;
  }
 };
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> nearUploads;
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> ringUploads;
 nearUploads.reserve(pendingMeshUploads_.size());
 ringUploads.reserve(pendingMeshUploads_.size());
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : pendingMeshUploads_) {
  if(job->nearLane) {
   nearUploads.push_back(std::move(job));
  } else {
   ringUploads.push_back(std::move(job));
  }
 }
 pendingMeshUploads_.clear();
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : meshScheduler_.drainCompleted()) {
  if(job->nearLane) {
   nearUploads.push_back(std::move(job));
  } else {
   ringUploads.push_back(std::move(job));
  }
 }
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : nearUploads) {
  processUpload(std::move(job), true);
 }
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : ringUploads) {
  processUpload(std::move(job), false);
 }
 pendingMeshUploads_ = std::move(deferredUploads);
 sweepRetiring();
 const std::size_t targetInFlight = workerCount * (force ? 6u : 3u);
 std::size_t inFlight = meshScheduler_.pendingJobs();
 if(inFlight < targetInFlight && pendingMeshUploads_.size() < targetInFlight * 2u) {
  const int requestedCaptures =
      client::option::chunkUpdatesPerPass(resolvedOpts, static_cast<int>(dirtyChunks_.size()));
  const int minCapturesPerFrame = force ? static_cast<int>(workerCount * 2u)
                                        : std::clamp(requestedCaptures, 1, static_cast<int>(workerCount));
  const net::minecraft::util::concurrent::FrameBudget captureBudget =
      net::minecraft::util::concurrent::FrameBudget::fromSharedMs((force || loadingBacklog) ? 2 : 1,
                                                                   minCapturesPerFrame);
  int captures = 0;
  const auto canCapture = [&] { return inFlight < targetInFlight && captureBudget.hasRemaining(captures); };
  for(auto it = nearDirtyChunks_.begin(); it != nearDirtyChunks_.end() && canCapture();) {
   chunk::ChunkBuilder* section = *it;
   if(section == nullptr || !section->dirty || section->meshJobInFlight) {
    it = nearDirtyChunks_.erase(it);
    continue;
   }
   if(startMeshJob(section, true, 0, resolvedOpts, fancyGraphics)) {
    it = nearDirtyChunks_.erase(it);
    ++inFlight;
    ++captures;
   } else {
    ++it;
   }
  }
  for(std::size_t ring = 0; ring < drawRings_.size() && canCapture(); ++ring) {
   for(chunk::ChunkBuilder* section : drawRings_[ring]) {
    if(!canCapture()) {
     break;
    }
    if(section == nullptr || !dirtyChunks_.contains(section)) {
     continue;
    }
    if(!section->dirty) {
     dirtyChunks_.erase(section);
     continue;
    }
    if(section->meshJobInFlight || (force && !section->inFrustum)) {
     continue;
    }
    if(startMeshJob(section, false, ringOf(*section), resolvedOpts, fancyGraphics)) {
     ++inFlight;
     ++captures;
    }
   }
  }
 }
 return dirtyChunks_.empty() && nearDirtyChunks_.empty() && pendingMeshUploads_.empty() && meshScheduler_.idle();
}
void WorldRenderer::renderEntities(const Vec3d& cameraPos,
                                   FrustumCuller* culler,
                                   float tickDelta) {
 if(entityRenderCooldown > 0) {
  --entityRenderCooldown;
  return;
 }
 if(world == nullptr || client == nullptr || cameraEntity_ == nullptr) {
  entityCount = 0;
  renderedEntityCount = 0;
  culledEntityCount = 0;
  return;
 }
 // Iris per-draw base: the full camera matrix the draw state publishes
 // (rotation * translate(camera - eye)); entity poses compose onto it. This is the
 // same matrix the old vanilla stack carried at draw time (see matrix.md).
 net::minecraft::util::math::MatrixStack matrices;
 matrices.load(core::drawModelView());
 const net::minecraft::util::math::Matrix4f& projection = core::drawProjection();
 auto* livingCamera = dynamic_cast<LivingEntity*>(cameraEntity_);
 auto& blockDispatcher = block::entity::BlockEntityRenderDispatcher::instance();
 blockDispatcher.prepare(world, textureManager, client->textRenderer.get(), cameraEntity_, tickDelta);
 auto& entityDispatcher = entity::EntityRenderDispatcher::instance();
 entityDispatcher.init(world, textureManager, client->textRenderer.get(), livingCamera, &activeOptions(), tickDelta);
 entityCount = 0;
 renderedEntityCount = 0;
 culledEntityCount = 0;
 if(livingCamera != nullptr) {
  double offsetX = 0.0;
  double offsetY = 0.0;
  double offsetZ = 0.0;
  cameraInterpPosition(static_cast<double>(tickDelta), offsetX, offsetY, offsetZ);
  entity::EntityRenderDispatcher::offsetX = offsetX;
  entity::EntityRenderDispatcher::offsetY = offsetY;
  entity::EntityRenderDispatcher::offsetZ = offsetZ;
  block::entity::BlockEntityRenderDispatcher::offsetX = entity::EntityRenderDispatcher::offsetX;
  block::entity::BlockEntityRenderDispatcher::offsetY = entity::EntityRenderDispatcher::offsetY;
  block::entity::BlockEntityRenderDispatcher::offsetZ = entity::EntityRenderDispatcher::offsetZ;
 }
 const std::vector<Entity*>& entities = world->entities();
 entityCount = static_cast<int>(entities.size()) + static_cast<int>(world->globalEntities.size());
 const client::option::RenderSettings& resolved = settings_;
 const FrameRenderCamera& renderCamera = RenderCameraState::instance().frame();
 const double shadowEntityDistanceSq =
     static_cast<double>(renderCamera.shadowEntityDistance) * renderCamera.shadowEntityDistance;
 const auto excludedFromShadow = [&](const Entity* entity) {
  if(!renderCamera.shadowPass || entity == nullptr) return false;
  if(!renderCamera.shadowEntities && client->player != nullptr &&
     entity != client->player && entity != client->player->vehicle) return true;
  if(!renderCamera.shadowPlayer && client->player != nullptr &&
     (entity == client->player || entity == client->player->vehicle)) return true;
  if(shadowEntityDistanceSq <= 0.0 || client->player == nullptr) return shadowEntityDistanceSq == 0.0;
  const double dx = entity->x - client->player->x;
  const double dy = entity->y - client->player->y;
  const double dz = entity->z - client->player->z;
  return dx * dx + dy * dy + dz * dz > shadowEntityDistanceSq;
 };
 for(Entity* entity : world->globalEntities) {
  if(entity == nullptr) {
   continue;
  }
  if(excludedFromShadow(entity)) continue;
  if(!client::option::shouldRenderEntity(resolved, *entity, cameraPos)) {
   ++culledEntityCount;
   continue;
  }
  ++renderedEntityCount;
  entityDispatcher.render(*entity, tickDelta, matrices, projection);
 }
 for(Entity* entity : entities) {
  if(entity == nullptr) {
   continue;
  }
  if(excludedFromShadow(entity)) continue;
  if(!client::option::shouldRenderEntity(resolved, *entity, cameraPos)) {
   ++culledEntityCount;
   continue;
  }
  if(!entity->ignoreFrustumCull && culler != nullptr && !culler->isVisible(entity->boundingBox)) {
   ++culledEntityCount;
   continue;
  }
  if(entity == cameraEntity_ && !renderCameraEntity_) {
   auto* playerCamera = dynamic_cast<PlayerEntity*>(cameraEntity_);
   if(playerCamera != nullptr && !activeOptions().thirdPerson && !playerCamera->isSleeping()) {
    continue;
   }
  }
  int blockY = MathHelper::floor(entity->y);
  if(blockY < 0) {
   blockY = 0;
  }
  if(blockY >= 128) {
   blockY = 127;
  }
  if(!world->isPosLoaded(MathHelper::floor(entity->x), blockY, MathHelper::floor(entity->z))) {
   continue;
  }
  ++renderedEntityCount;
  entityDispatcher.render(*entity, tickDelta, matrices, projection);
 }
 for(::net::minecraft::block::entity::BlockEntity* blockEntity : globalBlockEntities) {
  if(blockEntity == nullptr) continue;
  if(renderCamera.shadowPass) {
   bool allow = renderCamera.shadowBlockEntities;
   if(!allow && renderCamera.shadowLightBlockEntities && world != nullptr) {
    const int id = world->getBlockId(blockEntity->x, blockEntity->y, blockEntity->z);
    if(id >= 0 && id < net::minecraft::block::Block::BLOCK_COUNT) {
     const net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(id)];
     allow = block != nullptr && block->emission() > 0;
    }
   }
   if(!allow) continue;
   if(shadowEntityDistanceSq > 0.0 && client->player != nullptr) {
    const double dx = blockEntity->x - client->player->x;
    const double dy = blockEntity->y - client->player->y;
    const double dz = blockEntity->z - client->player->z;
    if(dx * dx + dy * dy + dz * dz > shadowEntityDistanceSq) continue;
   }
  }
  blockDispatcher.render(*blockEntity, tickDelta);
 }
}
void WorldRenderer::cullChunks(FrustumCuller* culler, float /*tickDelta*/, bool updateFrontier) {
 const FrameRenderCamera& renderCamera = RenderCameraState::instance().frame();
 const double nearFrustumBypassBlocks = std::max(0.0f, renderCamera.frustumBypassDistance);
 const double nearFrustumBypassDistanceSq = nearFrustumBypassBlocks * nearFrustumBypassBlocks;
 const double shadowRenderDistance = std::max(0.0f, renderCamera.shadowRenderDistance);
 const double shadowRenderDistanceSq = shadowRenderDistance * shadowRenderDistance;
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
  if(culler == nullptr || !settings_.frustumCulling) {
  for(chunk::ChunkBuilder* chunk : sectionList_) {
   chunk->inFrustum = true;
   if(renderCamera.shadowPass && shadowRenderDistanceSq > 0.0) {
    const double dx = camX - static_cast<double>(chunk->centerX);
    const double dy = camY - static_cast<double>(chunk->centerY);
    const double dz = camZ - static_cast<double>(chunk->centerZ);
    if(dx * dx + dy * dy + dz * dz > shadowRenderDistanceSq) {
     chunk->inFrustum = false;
    }
   }
  }
  rebuildVisibleDrawRings();
  return;
 }
 for(chunk::ChunkBuilder* chunkPtr : sectionList_) {
  chunk::ChunkBuilder& chunk = *chunkPtr;
  chunk.updateFrustum(*culler);
  const double dx = camX - static_cast<double>(chunk.centerX);
  const double dy = camY - static_cast<double>(chunk.centerY);
  const double dz = camZ - static_cast<double>(chunk.centerZ);
  const double distSq = dx * dx + dy * dy + dz * dz;
  if(dx * dx + dz * dz <= nearFrustumBypassDistanceSq) {
   chunk.inFrustum = true;
  }
  if(renderCamera.shadowPass && shadowRenderDistanceSq > 0.0 && distSq > shadowRenderDistanceSq) {
   chunk.inFrustum = false;
  }
 }
  // Light-space occlusion is invalid for the shadow pass (player-viewpoint flood
  // fill); the frustum loop above already applied the shadow render-distance cut.
  // pushCullState() snapshotted the main-pass inFrustum flags, so popCullState()
  // restores them unchanged, leaving the main pass unaffected.
  if(!renderCamera.shadowPass) {
   applyOcclusionCulling();
  }
  rebuildVisibleDrawRings();
}
void WorldRenderer::applyOcclusionCulling() {
 if(!settings_.occlusionCulling) {
  return;
 }
 constexpr double kNearOcclusionBypassSq = 48.0 * 48.0;
 const auto& frameCam = RenderCameraState::instance().frame();
 double camX = frameCam.x;
 double camY = frameCam.y;
 double camZ = frameCam.z;
 if(camX == 0.0 && camY == 0.0 && camZ == 0.0) {
  if(cameraEntity_ != nullptr) {
   camX = cameraEntity_->x;
   camY = cameraEntity_->y;
   camZ = cameraEntity_->z;
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
std::string WorldRenderer::getChunkDebugInfo() const {
 return "Chunks drawn: " + std::to_string(compiledChunkCount) + " of " + std::to_string(chunkCount) +
        ". Frustum-culled: " + std::to_string(invisibleChunkCount) + ", Empty: " +
        std::to_string(emptyChunkCount) + ", Draw calls: " +
        std::to_string(chunk::ChunkRegionBuffer::frameDrawCalls) + "/" +
        std::to_string(chunk::ChunkRegionBuffer::frameVisibleRanges);
}
std::string WorldRenderer::getEntityDebugInfo() const {
 return "Entities rendered: " + std::to_string(renderedEntityCount) + " of " +
        std::to_string(entityCount) + ". Frustum-culled: " + std::to_string(culledEntityCount) +
        ", Hidden: " + std::to_string(entityCount - culledEntityCount - renderedEntityCount);
}
void WorldRenderer::markDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
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
    enqueueDirtyChunk(builder);
   }
  }
 }
}
void WorldRenderer::blockUpdate(int x, int y, int z) {
 markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}
void WorldRenderer::chunkAvailable(int chunkX, int chunkZ) {
 if(centerSectionX_ == std::numeric_limits<int>::min()) {
  return;
 }
 // Defensive: never gate/enqueue a column whose chunk is not actually loaded
 // (an eviction may have raced the notification).
 if(world == nullptr || world->getChunkSource() == nullptr ||
    !world->getChunkSource()->isChunkLoaded(chunkX, chunkZ)) {
  return;
 }
 // Allow one extra ring: a column just outside the render radius still shares a
 // border with one just inside it, and that inside column's faces were meshed
 // against the missing data.
 if(std::abs(chunkX - centerSectionX_) > renderRadiusChunks_ + 1 ||
    std::abs(chunkZ - centerSectionZ_) > renderRadiusChunks_ + 1) {
  return;
 }
  lightingGate_.gate(chunkX, chunkZ);
  enqueueColumn(chunkX, chunkZ);
  pendingBorderRefresh_.insert(world::SectionPos{chunkX, 0, chunkZ});
}
// The chunk cache evicted a column. Drop the mirroring sections (plus any stale
// gate / border-refresh bookkeeping) so the renderer never keeps meshes for data
// that is gone, and a later reload rebuilds the column from scratch instead of
// reusing stale state.
void WorldRenderer::chunkUnloaded(int chunkX, int chunkZ) {
 lightingGate_.release(chunkX, chunkZ);
 pendingBorderRefresh_.erase(world::SectionPos{chunkX, 0, chunkZ});
 removeColumn(chunkX, chunkZ);
}
// P-LITGATE: the column's lighting drained (World::doLightingUpdates). Release
// its gate and re-enqueue the unbuilt sections whose first mesh was held.
void WorldRenderer::markChunkColumnLit(int chunkX, int chunkZ) {
 lightingGate_.release(chunkX, chunkZ);
 for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
  chunk::ChunkBuilder* section = sectionAt(chunkX, sectionY, chunkZ);
  if(section != nullptr && section->dirty && !section->built) {
   enqueueDirtyChunk(section);
  }
 }
}
// P-LITGATE: the lighting engine went fully idle — no remaining boxes could
// produce a region for a held column, so every gate is released (non-optional
// completion; a column with no pending boxes is never held forever).
void WorldRenderer::markAllChunksLit() {
 if(lightingGate_.empty()) {
  return;
 }
 std::vector<world::SectionPos> columns;
 columns.reserve(lightingGate_.pending_.size());
 for(const world::SectionPos& column : lightingGate_.pending_) {
  columns.push_back(column);
 }
 lightingGate_.releaseAll();
 for(const world::SectionPos& column : columns) {
  for(int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
   chunk::ChunkBuilder* section = sectionAt(column.x, sectionY, column.z);
   if(section != nullptr && section->dirty && !section->built) {
    enqueueDirtyChunk(section);
   }
  }
 }
}
void WorldRenderer::drainBorderRefresh() {
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
    enqueueDirtyChunk(section);
   }
  }
 }
 pendingBorderRefresh_.clear();
}
void WorldRenderer::setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 markDirty(minX - 1, minY - 1, minZ - 1, maxX + 1, maxY + 1, maxZ + 1);
}
void WorldRenderer::addParticle(
    const std::string& particle, double x, double y, double z, double velocityX, double velocityY, double velocityZ) {
 if(client == nullptr) {
  return;
 }
 if(!client::option::shouldSpawnParticle(settings_, particle)) {
  return;
 }
 net::minecraft::Entity* camera = cameraEntity_ != nullptr ? cameraEntity_ : client->camera;
 if(camera == nullptr) {
  return;
 }
 const double dx = camera->x - x;
 const double dy = camera->y - y;
 const double dz = camera->z - z;
 if(dx * dx + dy * dy + dz * dz > 16.0 * 16.0) {
  return;
 }
 client::particle::ParticleSpawnContext context{world, textureManager, x, y, z, velocityX, velocityY, velocityZ};
 std::unique_ptr<client::particle::Particle> spawned =
     client::particle::ParticleRegistry::instance().create(particle, context);
 if(spawned != nullptr) {
  client->particleManager.addParticle(std::move(spawned));
 }
}
void WorldRenderer::notifyEntityAdded(net::minecraft::Entity* entity) {
 if(entity == nullptr || client == nullptr) {
  return;
 }
 entity->updateCapeUrl();
 if(!entity->skinUrl.empty()) {
  client->textureManager.downloadSkinImage(entity->skinUrl);
 }
 if(!entity->capeUrl.empty()) {
  client->textureManager.downloadCapeImage(entity->capeUrl);
 }
}
void WorldRenderer::notifyEntityRemoved(net::minecraft::Entity* entity) {
 if(entity == nullptr || client == nullptr) {
  return;
 }
 if(!entity->skinUrl.empty()) {
  client->textureManager.releaseImage(entity->skinUrl);
 }
 if(!entity->capeUrl.empty()) {
  client->textureManager.releaseImage(entity->capeUrl);
 }
}
void WorldRenderer::notifyAmbientDarknessChanged() {
 for(auto& entry : sections_) {
  chunk::ChunkBuilder& chunk = *entry.second;
  if(!chunk.hasSkyLight || chunk.dirty) {
   continue;
  }
  chunk.invalidate();
  enqueueDirtyChunk(&chunk);
 }
}
void WorldRenderer::updateBlockEntity(int x,
                                      int y,
                                      int z,
                                      net::minecraft::block::entity::BlockEntity* /*blockEntity*/) {
 markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}
void WorldRenderer::onEntityPickup(net::minecraft::Entity* entity, net::minecraft::PlayerEntity* collector) {
 if(client == nullptr || world == nullptr || entity == nullptr || collector == nullptr) {
  return;
 }
 client->particleManager.addParticle(
     new ::net::minecraft::client::particle::PickupParticle(world, entity, collector, -0.5f));
}
void WorldRenderer::blockBreakParticles(int x, int y, int z, int blockId, int blockMeta) {
 if(client == nullptr) {
  return;
 }
 client->particleManager.addBlockBreakParticles(x, y, z, blockId, blockMeta);
}
void WorldRenderer::renderOutline(const Box& box) {
 Tessellator& tessellator = INSTANCE;
 auto emitStrip = [&](bool top) {
  const double y = top ? box.maxY : box.minY;
  tessellator.start(gl::prim::LineStrip);
  tessellator.color(0.0f, 0.0f, 0.0f, 0.4f);
  tessellator.vertex(box.minX, y, box.minZ);
  tessellator.vertex(box.maxX, y, box.minZ);
  tessellator.vertex(box.maxX, y, box.maxZ);
  tessellator.vertex(box.minX, y, box.maxZ);
  tessellator.vertex(box.minX, y, box.minZ);
  tessellator.draw();
 };
 emitStrip(false);
 emitStrip(true);
 tessellator.start(gl::prim::Lines);
 tessellator.color(0.0f, 0.0f, 0.0f, 0.4f);
 tessellator.vertex(box.minX, box.minY, box.minZ);
 tessellator.vertex(box.minX, box.maxY, box.minZ);
 tessellator.vertex(box.maxX, box.minY, box.minZ);
 tessellator.vertex(box.maxX, box.maxY, box.minZ);
 tessellator.vertex(box.maxX, box.minY, box.maxZ);
 tessellator.vertex(box.maxX, box.maxY, box.maxZ);
 tessellator.vertex(box.minX, box.minY, box.maxZ);
 tessellator.vertex(box.minX, box.maxY, box.maxZ);
 tessellator.draw();
}
void WorldRenderer::renderMiningProgress(net::minecraft::PlayerEntity* player,
                                         const net::minecraft::HitResult& hitResult,
                                         int i,
                                         const net::minecraft::ItemStack& handStack,
                                         float tickDelta) {
 (void)handStack;
 if(i != 0 || hitResult.type != HitResultType::BLOCK || player == nullptr || world == nullptr) {
  return;
 }
 if(miningProgress <= 0.0f || miningProgress > 1.0f) {
  return;
 }
 const int blockId = world->getBlockId(hitResult.blockX, hitResult.blockY, hitResult.blockZ);
 if(blockId <= 0 || blockId >= Block::BLOCK_COUNT || Block::BLOCKS[blockId] == nullptr) {
  return;
 }
 Block* block = Block::BLOCKS[blockId];
 int stage = static_cast<int>(miningProgress * 10.0f);
 if(stage < 0) {
  stage = 0;
 }
 if(stage > 9) {
  stage = 9;
 }
 const int destroyTexture = 240 + stage;
 double interpX = 0.0, interpY = 0.0, interpZ = 0.0;
 cameraInterpPosition(static_cast<double>(tickDelta), interpX, interpY, interpZ);
 const RenderPassScope passScope(RenderType::damagedBlock());
 net::minecraft::client::texture::TextureManager* texMgr =
     textureManager != nullptr ? textureManager : (client != nullptr ? &client->textureManager : nullptr);
  if(texMgr != nullptr) {
   texMgr->bindTexture(texMgr->getTextureId("/terrain.png"));
  }
 core::enableBlend();
 core::blendAlpha();
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 core::depthMask(false);
 core::depthTest();
 core::polygonOffset(-3.0f, -3.0f);
 core::enablePolygonOffset();
 blockRenderManager.snapshotGlobals();
 blockRenderManager.ctx.blockView = world;
 blockRenderManager.ctx.textureManager = texMgr;
 blockRenderManager.ctx.skipFaceCulling = true;
 Tessellator& tess = Tessellator::INSTANCE;
 tess.startQuads();
 tess.translate(-interpX, -interpY, -interpZ);
 tess.color(1.0f, 1.0f, 1.0f, 1.0f);
 blockRenderManager.renderWithTexture(
     *block, hitResult.blockX, hitResult.blockY, hitResult.blockZ, destroyTexture);
 tess.draw();
 tess.translate(0.0, 0.0, 0.0);
 blockRenderManager.ctx.skipFaceCulling = false;
 core::disablePolygonOffset();
 core::depthMask(true);
 core::disableBlend();
}
void WorldRenderer::renderBlockOutline(net::minecraft::PlayerEntity* player,
                                       const net::minecraft::HitResult& hitResult,
                                       int i,
                                       const net::minecraft::ItemStack& handStack,
                                       float tickDelta) {
 (void)handStack;
 if(i != 0 || hitResult.type != HitResultType::BLOCK || player == nullptr || world == nullptr) {
  return;
 }
 const BlockOutlineScope outlineCaps;
 core::setEntityColor(0.0f, 0.0f, 0.0f, 0.0f);
 Tessellator::INSTANCE.light(15, 15);
 ::glLineWidth(1.0f);
 core::depthMask(false);
 constexpr float expand = 0.002f;
 const int blockId = world->getBlockId(hitResult.blockX, hitResult.blockY, hitResult.blockZ);
 if(blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[blockId] != nullptr) {
  Block* block = Block::BLOCKS[blockId];
  block->updateBoundingBox(world, hitResult.blockX, hitResult.blockY, hitResult.blockZ);
  double interpX = 0.0, interpY = 0.0, interpZ = 0.0;
  cameraInterpPosition(static_cast<double>(tickDelta), interpX, interpY, interpZ);
  Box outline = block->getBoundingBox(world, hitResult.blockX, hitResult.blockY, hitResult.blockZ);
  outline = outline.expand(expand).offset(-interpX, -interpY, -interpZ);
  renderOutline(outline);
 }
}
void WorldRenderer::cameraInterpPosition(double tickDelta, double& x, double& y, double& z) const {
 // The render origin is the iris frame camera position (GameRenderer publishes
 // RenderCameraState before world rendering); the vanilla camera-entity fallback
 // duplicated the same interpolation.
 (void)tickDelta;
 const auto& frameCam = RenderCameraState::instance().frame();
 x = frameCam.x;
 y = frameCam.y;
 z = frameCam.z;
}
void WorldRenderer::releaseSections() {
 clearSections();
}
} // namespace net::minecraft::client::render
