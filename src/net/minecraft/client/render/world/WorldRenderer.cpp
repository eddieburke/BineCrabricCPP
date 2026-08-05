#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/ClientLog.hpp"
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
    // Pre-size each terrain region's VBO so the initial chunk stream does not
    // repeatedly grow the buffers and re-upload every accumulated range (a
    // per-frame stall on world load / teleport). Estimate ~384
    // vertices/section/layer at steady state, split across the region grid,
    // capped so a giant view distance does not reserve gigabytes up front.
    if(chunkSections_.renderRadiusChunks() > 0) {
     const std::size_t columns = static_cast<std::size_t>(2 * chunkSections_.renderRadiusChunks() + 1);
     const std::size_t sections = columns * columns * static_cast<std::size_t>(kChunkSectionCountY);
     const std::size_t regionColumns = (columns + chunk::kRegionSectionsX - 1) / chunk::kRegionSectionsX;
     const std::size_t regionRows = (static_cast<std::size_t>(kChunkSectionCountY) + chunk::kRegionSectionsY - 1) /
                                    chunk::kRegionSectionsY;
     const std::size_t regionCount =
         std::max<std::size_t>(1, regionColumns * regionColumns * regionRows);
     const std::size_t estimate = std::min<std::size_t>(sections * 384u, 16u * 1024u * 1024u);
     const std::size_t perRegion =
         std::clamp<std::size_t>(estimate / regionCount, 1u << 16, 1u << 20);
     compilePipeline_.regionManager().setReserveHint(perRegion);
    }
  globalBlockEntities.clear();
  entityRenderCooldown = 2;
}
void WorldRenderer::reloadIfViewDistanceChanged() {
 chunkSections_.reloadIfViewDistanceChanged();
}
int WorldRenderer::render(net::minecraft::LivingEntity& camera, int layer, bool drawModMeshes) {
 if(chunkSections_.empty()) {
  return 0;
 }
 cameraEntity_ = &camera;
 renderChunksVbo(layer, false);
 if(drawModMeshes) {
  renderModChunkMeshes(layer);
 }
 return 0;
}
void WorldRenderer::renderLastChunks(int layer, double /*tickDelta*/) {
 // Java's GameRenderer draws translucent terrain twice when fancyGraphics is
 // on: once with colorMask off (depth-only prepass) via render(), then again
 // here with colorMask restored so blending reads back a depth buffer that
   // already matches, avoiding z-fighting between overlapping translucent
   // faces (water against glass, etc). Re-running the same layer draw covers
   // the VBO region path.
  renderChunksVbo(layer, true);
  renderModChunkMeshes(layer);
}
void WorldRenderer::sectionOrigin(double& x, double& y, double& z) noexcept {
 if(core::drawCameraStateValid()) {
  const float* eye = core::drawCameraPosition();
  x = eye[0];
  y = eye[1];
  z = eye[2];
  return;
 }
 const FrameRenderCamera& cam = RenderCameraState::instance().frame();
 x = cam.eyeX;
 y = cam.eyeY;
 z = cam.eyeZ;
}
int WorldRenderer::renderChunksVbo(int layer, bool skipBuildDrawLists) {
 // chunkOffset = sectionOrigin - the active draw camera (player or shadow). One
 // origin for everything: sectionOrigin() is the same point cullChunks culls
 // against. Each region draws with its own origin, so the offset is uniform per
 // region (regionOrigin - camera) instead of per section.
 double camX = 0.0;
 double camY = 0.0;
 double camZ = 0.0;
 sectionOrigin(camX, camY, camZ);
 if(!skipBuildDrawLists) {
  // Reset on the first layer of the player's own pass only. Nested passes
  // (sun shadow) set renderCameraEntity_, and letting
  // them reset would leave the F3 counter showing the shadow map's draw calls
  // instead of the frame's.
  if(layer == 0 && !renderCameraEntity_) {
   chunk::ChunkRegionBuffer::frameVisibleRanges = 0;
   chunk::ChunkRegionBuffer::frameDrawCalls = 0;
  }
  compilePipeline_.regionManager().forEachRegion([layer, camX, camY, camZ](const chunk::RegionKey& key,
                                                                           chunk::ChunkRegion& region) {
   chunk::ChunkRegionBuffer& buffer = region.layers[static_cast<std::size_t>(layer)];
   buffer.beginFrame(static_cast<float>(static_cast<double>(key.x) * chunk::kRegionBlocksX - camX),
                     static_cast<float>(static_cast<double>(key.y) * chunk::kRegionBlocksY - camY),
                     static_cast<float>(static_cast<double>(key.z) * chunk::kRegionBlocksZ - camZ));
  });
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
    chunk->region_->layers[static_cast<std::size_t>(layer)].addVisible(slot);
   }
  }
 }
 // TEMP DIAGNOSTIC — [terrain-probe]. Arm on the solid layer of the player's own
 // pass; see ChunkRegionBuffer.hpp probeArmed.
 static int terrainProbeFrame = 0;
 static int terrainProbeShadowFrame = 0;
 const bool probeThisPass =
     layer == 0 && ((renderCameraEntity_ ? terrainProbeShadowFrame++ : terrainProbeFrame++) % 120) == 0;
 const char* const probePass = renderCameraEntity_ ? "shadow" : "player";
 if(probeThisPass) {
  chunk::ChunkRegionBuffer::probeArmed = true;
  chunk::ChunkRegionBuffer::probeRegions = 0;
  chunk::ChunkRegionBuffer::probeRanges = 0;
  chunk::ChunkRegionBuffer::probeVertices = 0;
  chunk::ChunkRegionBuffer::probeAny = false;
 }
 int draws = 0;
 compilePipeline_.regionManager().forEachRegion(
     [layer, &draws](const chunk::RegionKey&, chunk::ChunkRegion& region) {
      draws += region.layers[static_cast<std::size_t>(layer)].flush();
     });
 if(probeThisPass) {
  chunk::ChunkRegionBuffer::probeArmed = false;
  const float* projection = chunk::ChunkRegionBuffer::probeProjection;
  const float* modelView = chunk::ChunkRegionBuffer::probeModelView;
  char line[512];
  std::snprintf(line,
                sizeof(line),
                "[terrain-probe] %s pass regions=%d ranges=%d verts=%lld camera=(%.2f,%.2f,%.2f) "
                "cameraRelativeAabb min=(%.1f,%.1f,%.1f) max=(%.1f,%.1f,%.1f)%s",
                probePass,
                chunk::ChunkRegionBuffer::probeRegions,
                chunk::ChunkRegionBuffer::probeRanges,
                chunk::ChunkRegionBuffer::probeVertices,
                camX,
                camY,
                camZ,
                static_cast<double>(chunk::ChunkRegionBuffer::probeMin[0]),
                static_cast<double>(chunk::ChunkRegionBuffer::probeMin[1]),
                static_cast<double>(chunk::ChunkRegionBuffer::probeMin[2]),
                static_cast<double>(chunk::ChunkRegionBuffer::probeMax[0]),
                static_cast<double>(chunk::ChunkRegionBuffer::probeMax[1]),
                static_cast<double>(chunk::ChunkRegionBuffer::probeMax[2]),
                chunk::ChunkRegionBuffer::probeAny ? "" : "  <-- NOTHING DRAWN");
  ClientLog::LOGGER.log(::net::minecraft::util::logging::LogLevel::Info, std::string(line));
  // m[11] == -1 is a perspective divide; 0 means an orthographic matrix reached
  // the player's pass (i.e. the shadow camera leaked). A zero m0 means nothing has
  // ever been uploaded yet (no draw ran), so the verdict is meaningless.
  std::snprintf(line,
                sizeof(line),
                "[terrain-probe] %s pass uploaded proj m0=%.4f m5=%.4f m10=%.4f m11=%.1f m14=%.4f m15=%.4f (%s)  "
                "modelView trans=(%.2f,%.2f,%.2f)",
                probePass,
                static_cast<double>(projection[0]),
                static_cast<double>(projection[5]),
                static_cast<double>(projection[10]),
                static_cast<double>(projection[11]),
                static_cast<double>(projection[14]),
                static_cast<double>(projection[15]),
                projection[0] == 0.0f
                    ? "nothing drawn (stale zeros)"
                    : renderCameraEntity_
                          ? (projection[11] < -0.5f ? "PERSPECTIVE -- expected ortho" : "ortho ok")
                          : (projection[11] < -0.5f ? "perspective ok" : "ORTHO -- SHADOW CAMERA LEAKED"),
                static_cast<double>(modelView[12]),
                static_cast<double>(modelView[13]),
                static_cast<double>(modelView[14]));
  ClientLog::LOGGER.log(::net::minecraft::util::logging::LogLevel::Info, std::string(line));
  // GPU truth at draw time: the polygon offset the shadow bias relies on, the depth
  // func, and the viewport, read back from the driver rather than our own cache.
  int polyFactor = 0;
  int polyUnits = 0;
  int depthFunc = 0;
  int viewport[4] = {0, 0, 0, 0};
  ::glGetIntegerv(0x8038, &polyFactor);
  ::glGetIntegerv(0x2A00, &polyUnits);
  ::glGetIntegerv(static_cast<unsigned>(gl::query::DepthFunc), &depthFunc);
  ::glGetIntegerv(static_cast<unsigned>(gl::query::Viewport), viewport);
  std::snprintf(line,
                sizeof(line),
                "[terrain-probe] %s pass GPU polyOffset=%s factor=%d units=%d depthFunc=0x%X "
                "viewport=(%d,%d,%d,%d)",
                probePass,
                ::glIsEnabled(0x8037) ? "ENABLED" : "DISABLED",
                polyFactor,
                polyUnits,
                depthFunc,
                viewport[0],
                viewport[1],
                viewport[2],
                viewport[3]);
  ClientLog::LOGGER.log(::net::minecraft::util::logging::LogLevel::Info, std::string(line));
 }
 return draws;
}
int WorldRenderer::renderModChunkMeshes(int layer) {
 if(textureManager == nullptr) {
  return 0;
 }
 const RenderType& renderType =
     layer == chunk::terrain_layer::Translucent      ? RenderType::translucent()
     : layer == chunk::terrain_layer::Cutout         ? RenderType::cutout()
     : layer == chunk::terrain_layer::CutoutInterior ? RenderType::cutoutInterior()
                                                     : RenderType::solid();
 const ModChunkMeshScope meshCaps(renderType);
 int drawn = 0;
 double camX = 0.0;
 double camY = 0.0;
 double camZ = 0.0;
 sectionOrigin(camX, camY, camZ);
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
  // Iris parity pose base: identity, so every pose the entity/block-entity
  // renderers publish on this stack is the pure model -> camera-relative
  // transform (they push translate(entity - eye) themselves via the dispatcher
  // offsets below). The camera matrix is never part of a pose; it is uploaded
  // per pass and per draw from the camera state alone.
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
 // One geometry origin: the camera EYE (Camera.getPosition()), the same point
 // terrain, chunkOffset, the cameraPosition uniform and the shadow map centre
 // all anchor on. Entity/block-entity producers emit camera-relative poses from
 // here, so their vertices reach the shader as worldPos - eye and a pack that
 // cuts gl_ModelViewMatrix to a mat3 loses nothing.
 sectionOrigin(x, y, z);
}
} // namespace net::minecraft::client::render
