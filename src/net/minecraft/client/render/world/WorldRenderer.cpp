#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <vector>
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
#include "net/minecraft/client/render/chunk/ChunkRegionBuffer.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/culling/ShadowFrustum.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::render {
namespace {
constexpr int kChunkSectionCountY = 8;
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
}
net::minecraft::client::option::GameOptions& WorldRenderer::activeOptions() const {
 if(options_ != nullptr) {
  return *options_;
 }
 if(client != nullptr) {
  return const_cast<net::minecraft::client::option::GameOptions&>(client->options);
 }
 static net::minecraft::client::option::GameOptions fallback;
 return fallback;
}
const option::RenderSettings& WorldRenderer::frameSettings() const {
 if(client != nullptr && client->gameRenderer != nullptr) {
  return client->gameRenderer->frameSettings();
 }
 static const option::RenderSettings fallback;
 return fallback;
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
  chunkSections_.clearSections();
  cameraEntity_ = nullptr;
 }
}
void WorldRenderer::reload() {
 chunkSections_.clearSections();
 if(world == nullptr) {
  return;
 }
 // Resolve options + pack bake flags before sections rebuild. Stale
 // separateAo/oldLighting from the previous pack made Smooth Lighting look
 // broken until the slider forced another remesh.
 const option::RenderSettings& resolved = frameSettings();
 net::minecraft::client::option::GameOptions& opts = activeOptions();
 if(Block::LEAVES != nullptr) {
  static_cast<net::minecraft::block::LeavesBlock*>(Block::LEAVES)->setFancyGraphics(resolved.fancyLeaves);
 }
 block::BlockRenderManager::fancyLeaves = resolved.fancyLeaves;
 blockRenderManager.snapshotGlobals();
 chunkSections_.setLastViewDistance(opts.viewDistance);
 chunkSections_.setLastRenderScale(resolved.renderScale);
  gl::GLCore::ensureLoaded();
  chunkSections_.setRenderRadius(resolved.chunkRadius);
   // Pre-size the shared terrain VBO so the initial chunk stream does not repeatedly
   // grow the buffer and re-upload every accumulated range (a per-frame stall on
   // world load / teleport). Estimate ~384 vertices/section/layer at steady state,
   // capped so a giant view distance does not reserve gigabytes up front.
   if(chunkSections_.renderRadiusChunks() > 0) {
    const std::size_t columns = static_cast<std::size_t>(2 * chunkSections_.renderRadiusChunks() + 1);
    const std::size_t sections = columns * columns * static_cast<std::size_t>(kChunkSectionCountY);
    const std::size_t estimate = std::min<std::size_t>(sections * 384u, 16u * 1024u * 1024u);
    chunk::ChunkRegion& region = compilePipeline_.regionManager().pool();
    for(auto& layer : region.layers) layer.reserve(estimate);
    net::minecraft::client::render::quad_index::ensure(estimate);
   }
  globalBlockEntities.clear();
  entityRenderCooldown = 2;
}
void WorldRenderer::reloadIfViewDistanceChanged() {
 chunkSections_.reloadIfViewDistanceChanged();
}
int WorldRenderer::render(net::minecraft::LivingEntity& camera, int layer, double tickDelta, bool drawModMeshes) {
 if(chunkSections_.empty()) {
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
 chunk::ChunkRegion& pool = compilePipeline_.regionManager().pool();
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
  for(const std::vector<chunk::ChunkBuilder*>& ring : chunkSections_.visibleDrawRings()) {
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
 for(const std::vector<chunk::ChunkBuilder*>& ring : chunkSections_.visibleDrawRings()) {
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
 const client::option::RenderSettings& resolved = frameSettings();
 const FrameRenderCamera& renderCamera = RenderCameraState::instance().frame();
 // Java culls shadow entities with the entity shadow frustum
 // (ShadowRenderer.renderEntities → entityShadowFrustum), the same extruded-player
 // volume as the terrain, optionally narrowed by entityShadowDistanceMul.
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
 const ShadowCullingFrustum* shadowEntityFrustum =
     renderCamera.shadowPass ? renderCamera.shadowEntityFrustum : nullptr;
 const auto excludedFromShadow = [&](const Entity* entity) {
  if(!renderCamera.shadowPass || entity == nullptr) return false;
  if(!renderCamera.shadowEntities && client->player != nullptr &&
     entity != client->player && entity != client->player->vehicle) return true;
  if(!renderCamera.shadowPlayer && client->player != nullptr &&
     (entity == client->player || entity == client->player->vehicle)) return true;
  return shadowEntityFrustum != nullptr && !entity->ignoreFrustumCull &&
         !shadowEntityFrustum->isVisible(entity->boundingBox);
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
   // Java block entities ride along with the terrain the shadow frustum kept, so cull
   // them against the terrain frustum rather than a distance sphere of our own.
   if(renderCamera.shadowTerrainFrustum != nullptr &&
      !renderCamera.shadowTerrainFrustum->isVisible(
          net::minecraft::Box(static_cast<double>(blockEntity->x), static_cast<double>(blockEntity->y),
                              static_cast<double>(blockEntity->z), static_cast<double>(blockEntity->x) + 1.0,
                              static_cast<double>(blockEntity->y) + 1.0,
                              static_cast<double>(blockEntity->z) + 1.0))) {
    continue;
   }
  }
  blockDispatcher.render(*blockEntity, tickDelta);
 }
}
bool WorldRenderer::compileChunks(net::minecraft::LivingEntity& camera, bool force) {
 return compilePipeline_.compileChunks(camera, force);
}
void WorldRenderer::cullChunks(FrustumCuller* culler, float tickDelta, bool updateFrontier) {
 chunkSections_.cullChunks(culler, tickDelta, updateFrontier);
}
void WorldRenderer::pushCullState() {
 chunkSections_.pushCullState();
}
void WorldRenderer::popCullState() {
 chunkSections_.popCullState();
}
void WorldRenderer::releaseSections() {
 chunkSections_.clearSections();
}
std::string WorldRenderer::getChunkDebugInfo() const {
 return chunkSections_.getChunkDebugInfo();
}
std::string WorldRenderer::getEntityDebugInfo() const {
 return "Entities rendered: " + std::to_string(renderedEntityCount) + " of " +
        std::to_string(entityCount) + ". Frustum-culled: " + std::to_string(culledEntityCount) +
        ", Hidden: " + std::to_string(entityCount - culledEntityCount - renderedEntityCount);
}
void WorldRenderer::markDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 chunkSections_.markDirty(minX, minY, minZ, maxX, maxY, maxZ);
}
void WorldRenderer::blockUpdate(int x, int y, int z) {
 chunkSections_.blockUpdate(x, y, z);
}
void WorldRenderer::setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 chunkSections_.setBlocksDirty(minX, minY, minZ, maxX, maxY, maxZ);
}
void WorldRenderer::chunkAvailable(int chunkX, int chunkZ) {
 chunkSections_.chunkAvailable(chunkX, chunkZ);
}
void WorldRenderer::chunkUnloaded(int chunkX, int chunkZ) {
 chunkSections_.chunkUnloaded(chunkX, chunkZ);
}
void WorldRenderer::markChunkColumnLit(int chunkX, int chunkZ) {
 chunkSections_.markChunkColumnLit(chunkX, chunkZ);
}
void WorldRenderer::markAllChunksLit() {
 chunkSections_.markAllChunksLit();
}
void WorldRenderer::notifyAmbientDarknessChanged() {
 chunkSections_.notifyAmbientDarknessChanged();
}
void WorldRenderer::updateBlockEntity(int x,
                                      int y,
                                      int z,
                                      net::minecraft::block::entity::BlockEntity* blockEntity) {
 chunkSections_.updateBlockEntity(x, y, z, blockEntity);
}
void WorldRenderer::addParticle(
    const std::string& particle, double x, double y, double z, double velocityX, double velocityY, double velocityZ) {
 if(client == nullptr) {
  return;
 }
 if(!client::option::shouldSpawnParticle(frameSettings(), particle)) {
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
 // Modern line rendering (Iris 26.1 line format) carries each line's direction in
 // vaNormal so packs can expand it to a screen-space width (RenderPearl
 // gbuffers_line.vsh: start = model, end = model + vaNormal, ±offset by gl_VertexID).
 // Emit every edge as an explicit GL_LINES segment with that direction; a constant
 // (0,0,0) normal degenerates normalize(0) into undefined geometry.
 tessellator.start(gl::prim::Lines);
 tessellator.color(0.0f, 0.0f, 0.0f, 0.4f);
 auto emitEdge = [&tessellator](double ax, double ay, double az, double bx, double by, double bz) {
  double dx = bx - ax, dy = by - ay, dz = bz - az;
  const double len = std::sqrt(dx * dx + dy * dy + dz * dz);
  if(len > 1.0e-12) {
   dx /= len;
   dy /= len;
   dz /= len;
  } else {
   dx = 1.0;
   dy = 0.0;
   dz = 0.0;
  }
  tessellator.normal(static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz));
  tessellator.vertex(ax, ay, az);
  tessellator.normal(static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz));
  tessellator.vertex(bx, by, bz);
 };
 emitEdge(box.minX, box.minY, box.minZ, box.maxX, box.minY, box.minZ);
 emitEdge(box.maxX, box.minY, box.minZ, box.maxX, box.minY, box.maxZ);
 emitEdge(box.maxX, box.minY, box.maxZ, box.minX, box.minY, box.maxZ);
 emitEdge(box.minX, box.minY, box.maxZ, box.minX, box.minY, box.minZ);
 emitEdge(box.minX, box.maxY, box.minZ, box.maxX, box.maxY, box.minZ);
 emitEdge(box.maxX, box.maxY, box.minZ, box.maxX, box.maxY, box.maxZ);
 emitEdge(box.maxX, box.maxY, box.maxZ, box.minX, box.maxY, box.maxZ);
 emitEdge(box.minX, box.maxY, box.maxZ, box.minX, box.maxY, box.minZ);
 emitEdge(box.minX, box.minY, box.minZ, box.minX, box.maxY, box.minZ);
 emitEdge(box.maxX, box.minY, box.minZ, box.maxX, box.maxY, box.minZ);
 emitEdge(box.maxX, box.minY, box.maxZ, box.maxX, box.maxY, box.maxZ);
 emitEdge(box.minX, box.minY, box.maxZ, box.minX, box.maxY, box.maxZ);
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
} // namespace net::minecraft::client::render
