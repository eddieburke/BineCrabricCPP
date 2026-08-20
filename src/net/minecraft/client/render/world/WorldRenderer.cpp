#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/debug/VTuneTrace.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/particle/ParticleRegistry.hpp"
#include "net/minecraft/client/particle/PickupParticle.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
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
#include "net/minecraft/world/chunk/ChunkSource.hpp"
namespace net::minecraft::client::render {
namespace {
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
 scene_.client = client;
 scene_.options = &activeOptions();
 scene_.settings = &frameSettings();
 scene_.reloadRequested = [this]() { reload(); };
 // The two systems' construction is mutually circular; each was built against
 // the scene alone and the peer references are wired here.
 chunkSections_.setCompilePipeline(compilePipeline_);
 compilePipeline_.setSectionSystem(chunkSections_);
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
 scene_.world = worldIn;
 blockRenderManager.setBlockView(worldIn);
 if(client != nullptr) {
  entity::EntityRenderDispatcher::instance().setWorld(worldIn);
 }
 if(world != nullptr) {
  world->addEventListener(this);
  reload();
 } else {
  chunkSections_.clearSections();
  terrainRegionDrawCount_ = 0;
  terrainRegionDrawLayer_ = -1;
  setCamera(nullptr);
 }
}
void WorldRenderer::reload() {
 chunkSections_.clearSections();
 terrainRegionDrawCount_ = 0;
 terrainRegionDrawLayer_ = -1;
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
 scene_.blockEntities.clear();
 entityRenderCooldown = 2;
 if(ChunkSource* source = world->getChunkSource(); source != nullptr) {
  source->forEachLoadedChunk([this](int chunkX, int chunkZ, Chunk&) {
   chunkSections_.chunkAvailable(chunkX, chunkZ);
  });
 }
}
int WorldRenderer::render(net::minecraft::LivingEntity& camera, int layer, bool drawModMeshes) {
 if(chunkSections_.empty()) {
  return 0;
 }
 setCamera(&camera);
 return renderChunkLayer(layer, false, drawModMeshes);
}
void WorldRenderer::renderLastChunks(int layer, double /*tickDelta*/) {
 if(chunkSections_.empty()) {
  return;
 }
 renderChunkLayer(layer, true, true);
}
Vec3d WorldRenderer::sectionOrigin() noexcept {
 if(core::drawCameraStateValid()) {
  const float* eye = core::drawCameraPosition();
  return {eye[0], eye[1], eye[2]};
 }
 const FrameRenderCamera& cam = core::cameraFrame();
 return {cam.eyeX, cam.eyeY, cam.eyeZ};
}
int WorldRenderer::renderChunkLayer(int layer, bool replay, bool drawModMeshes) {
 // Replaying batches that were built for a different layer would draw one
 // layer's allocations out of another layer's arena, and batches built before
 // a reload point at freed regions. Both are caught here.
 if(replay && terrainRegionDrawLayer_ != layer) {
  return 0;
 }
 const Vec3d camPos = sectionOrigin();
 const bool translucent = layer == chunk::terrain_layer::Translucent;
 if(!replay) {
  if(layer == 0 && !renderCameraEntity_) {
   chunk::ChunkBuilder::frameDrawCalls = 0;
  }
  modMeshDraws_.clear();
  terrainRegionDrawCount_ = 0;
  terrainRegionDrawLayer_ = layer;
  const auto& visible = chunkSections_.visibleSections();
  // cullChunks already filtered by frustum on the way into `visible`; re-testing
  // visibility here only re-read an answer it had just written.
  chunk::TerrainRegion* lastRegion = nullptr;
  std::size_t lastRegionIndex = 0;
  const auto visit = [&](chunk::ChunkBuilder* chunk) {
   if(chunk == nullptr) {
    return;
   }
   const chunk::TerrainAllocation& allocation = chunk->terrainAllocation(layer);
   if(allocation.valid() && chunk->terrainRegion() != nullptr) {
    chunk::TerrainRegion* region = chunk->terrainRegion();
    // Sections arrive grouped by region, so the previous hit is almost always the
    // right one; without this memo every section rescanned the whole draw list.
    std::size_t index = lastRegionIndex;
    if(region != lastRegion) {
     index = 0;
     while(index < terrainRegionDrawCount_ && terrainRegionDraws_[index].region != region) ++index;
     if(index == terrainRegionDrawCount_) {
      ++terrainRegionDrawCount_;
      if(index == terrainRegionDraws_.size()) terrainRegionDraws_.push_back({});
      terrainRegionDraws_[index].region = region;
      terrainRegionDraws_[index].allocations.clear();
     }
     lastRegion = region;
     lastRegionIndex = index;
    }
    terrainRegionDraws_[index].allocations.push_back(&allocation);
   }
   if(textureManager != nullptr) {
    for(const chunk::ModChunkMesh& modMesh : chunk->modLayerMeshes_[static_cast<std::size_t>(layer)]) {
     if(!modMesh.mesh.empty()) {
      modMeshDraws_.push_back(
          ModMeshDraw{&modMesh,
                      static_cast<float>(static_cast<double>(chunk->x) - camPos.x),
                      static_cast<float>(static_cast<double>(chunk->y) - camPos.y),
                      static_cast<float>(static_cast<double>(chunk->z) - camPos.z)});
     }
    }
   }
  };
  if(translucent) {
   for(auto it = visible.rbegin(); it != visible.rend(); ++it) visit(*it);
  } else {
   for(chunk::ChunkBuilder* chunk : visible) visit(chunk);
  }
  if(translucent) {
   // Region-granular back-to-front; within a region the sections keep the
   // reverse visit order established above.
   constexpr double halfX = world::kRegionSectionsX * chunk::kSectionBlocks * 0.5;
   constexpr double halfY = world::kRegionSectionsY * chunk::kSectionBlocks * 0.5;
   constexpr double halfZ = world::kRegionSectionsZ * chunk::kSectionBlocks * 0.5;
   const auto depthSq = [&](const TerrainRegionDraw& draw) {
    const double dx = static_cast<double>(draw.region->originX()) + halfX - camPos.x;
    const double dy = static_cast<double>(draw.region->originY()) + halfY - camPos.y;
    const double dz = static_cast<double>(draw.region->originZ()) + halfZ - camPos.z;
    return dx * dx + dy * dy + dz * dz;
   };
   std::sort(terrainRegionDraws_.begin(),
             terrainRegionDraws_.begin() + static_cast<std::ptrdiff_t>(terrainRegionDrawCount_),
             [&](const TerrainRegionDraw& a, const TerrainRegionDraw& b) {
              return depthSq(a) > depthSq(b);
             });
  } else {
   std::sort(modMeshDraws_.begin(), modMeshDraws_.end(), [](const ModMeshDraw& a, const ModMeshDraw& b) {
    return a.mesh->texture < b.mesh->texture;
   });
  }
 }
 int draws = 0;
 for(std::size_t i = 0; i < terrainRegionDrawCount_; ++i) {
  TerrainRegionDraw& draw = terrainRegionDraws_[i];
  if(draw.region == nullptr || draw.allocations.empty()) continue;
  const float ox = static_cast<float>(static_cast<double>(draw.region->originX()) - camPos.x);
  const float oy = static_cast<float>(static_cast<double>(draw.region->originY()) - camPos.y);
  const float oz = static_cast<float>(static_cast<double>(draw.region->originZ()) - camPos.z);
  core::setPendingTerrainDraw(ox, oy, oz);
  const int submitted = draw.region->drawLayer(layer, draw.allocations);
  core::clearPendingTerrainDraw();
  draws += submitted;
  chunk::ChunkBuilder::frameDrawCalls += submitted;
 }
 if(!drawModMeshes) {
  return draws;
 }
 int boundTextureId = -1;
 int boundGlId = -1;
 for(const ModMeshDraw& draw : modMeshDraws_) {
  if(draw.mesh->texture != boundTextureId) {
   boundTextureId = draw.mesh->texture;
   boundGlId = net::minecraft::registry::TextureRegistry::resolveGlId(boundTextureId, *textureManager);
   if(boundGlId >= 0) textureManager->bindTexture(boundGlId);
  }
  if(boundGlId < 0) continue;
  core::setPendingTerrainDraw(draw.x, draw.y, draw.z);
  Tessellator::drawMesh(draw.mesh->mesh);
  core::clearPendingTerrainDraw();
  ++draws;
 }
 return draws;
}
void WorldRenderer::renderEntities(const Vec3d& cameraPos,
                                   Frustum* culler,
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
 net::minecraft::util::math::MatrixStack matrices;
 matrices.load(net::minecraft::util::math::Matrix4f::identityMatrix());
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
  matrixStackOrigin(offsetX, offsetY, offsetZ);
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
 const FrameRenderCamera& renderCamera = core::cameraFrame();
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
  VT_TRACE_COUNTER("EntityTraversals", 1);
  if(entity == nullptr) {
   continue;
  }
  if(excludedFromShadow(entity)) continue;
  if(!client::option::shouldRenderEntity(resolved, *entity, cameraPos)) {
   ++culledEntityCount;
   VT_TRACE_COUNTER("EntityCulled", 1);
   continue;
  }
  ++renderedEntityCount;
  VT_TRACE_COUNTER("EntityVisible", 1);
  entityDispatcher.render(*entity, tickDelta, matrices, projection);
 }
 for(Entity* entity : entities) {
  VT_TRACE_COUNTER("EntityTraversals", 1);
  if(entity == nullptr) {
   continue;
  }
  if(excludedFromShadow(entity)) continue;
  if(!client::option::shouldRenderEntity(resolved, *entity, cameraPos)) {
   ++culledEntityCount;
   VT_TRACE_COUNTER("EntityCulled", 1);
   continue;
  }
  if(!entity->ignoreFrustumCull && culler != nullptr && !culler->isVisible(entity->boundingBox)) {
   ++culledEntityCount;
   VT_TRACE_COUNTER("EntityCulled", 1);
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
  VT_TRACE_COUNTER("EntityVisible", 1);
  entityDispatcher.render(*entity, tickDelta, matrices, projection);
 }
 for(::net::minecraft::block::entity::BlockEntity* blockEntity : scene_.blockEntities) {
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
 Tessellator& tessellator = Tessellator::INSTANCE;
 // see shaders/ComplementaryReimagined_r5.8.1/shaders/program/gbuffers_line.vsh
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
 matrixStackOrigin(interpX, interpY, interpZ);
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
 // Damage overlay vertices are emitted camera-eye relative (translate(-eye)
 // below); an identity pose routes them through the camera matrix.
 core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
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
 // Outline vertices are emitted camera-eye relative; an identity pose routes
 // them through the camera matrix like terrain.
 core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
 constexpr float expand = 0.002f;
 const int blockId = world->getBlockId(hitResult.blockX, hitResult.blockY, hitResult.blockZ);
 if(blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[blockId] != nullptr) {
  Block* block = Block::BLOCKS[blockId];
  block->updateBoundingBox(world, hitResult.blockX, hitResult.blockY, hitResult.blockZ);
  double interpX = 0.0, interpY = 0.0, interpZ = 0.0;
  matrixStackOrigin(interpX, interpY, interpZ);
  Box outline = block->getBoundingBox(world, hitResult.blockX, hitResult.blockY, hitResult.blockZ);
  outline = outline.expand(expand).offset(-interpX, -interpY, -interpZ);
  renderOutline(outline);
 }
}
void WorldRenderer::matrixStackOrigin(double& x, double& y, double& z) const {
 const Vec3d origin = sectionOrigin();
 x = origin.x;
 y = origin.y;
 z = origin.z;
}
} // namespace net::minecraft::client::render
