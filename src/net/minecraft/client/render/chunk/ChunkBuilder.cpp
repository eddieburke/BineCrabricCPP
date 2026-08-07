#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <unordered_set>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/LeafInteriorFaces.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/pipeline/Manager.hpp"
#include "net/minecraft/util/concurrent/ThreadNames.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
namespace net::minecraft::client::render::chunk {
namespace {
class RenderPinGuard {
 public:
 explicit RenderPinGuard(std::vector<RegionSnapshot::SourceChunk>& pinned) noexcept : pinned_(pinned) {
 }
 ~RenderPinGuard() {
  release();
 }
 void disarm() noexcept {
  armed_ = false;
 }

 private:
 void release() noexcept {
  if(!armed_) {
   return;
  }
  for(const RegionSnapshot::SourceChunk& sourceChunk : pinned_) {
   if(sourceChunk.chunk != nullptr) {
    const_cast<Chunk*>(sourceChunk.chunk)->releaseRenderPin();
   }
  }
  pinned_.clear();
 }
 std::vector<RegionSnapshot::SourceChunk>& pinned_;
 bool armed_ = true;
};
// Flood-fills the section's non-opaque cells and records which face pairs a
// component connects (bit a*6+b). Faces: 0:-X 1:+X 2:-Y 3:+Y 4:-Z 5:+Z.
std::uint64_t computeVisibilityBits(const RegionSnapshot& snapshot, int minX, int minY, int minZ) {
 std::array<bool, 4096> opaque{};
 std::array<std::int8_t, 256> opaqueForId{};
 opaqueForId.fill(-1);
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   for(int y = 0; y < 16; ++y) {
    const int blockId = snapshot.getBlockId(minX + x, minY + y, minZ + z);
    bool cellOpaque = false;
    if(blockId > 0) {
     std::int8_t& cached = opaqueForId[static_cast<std::size_t>(blockId & 0xFF)];
     if(cached < 0) {
      net::minecraft::block::Block* block =
          net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
      cached = (block != nullptr && block->isOpaque()) ? 1 : 0;
     }
     cellOpaque = cached == 1;
    }
    opaque[static_cast<std::size_t>((y << 8) | (z << 4) | x)] = cellOpaque;
   }
  }
 }
 std::array<bool, 4096> visited{};
 std::uint64_t bits = 0;
 std::vector<int> stack;
 stack.reserve(1024);
 for(int start = 0; start < 4096; ++start) {
  if(visited[static_cast<std::size_t>(start)] || opaque[static_cast<std::size_t>(start)]) {
   continue;
  }
  unsigned faceMask = 0;
  visited[static_cast<std::size_t>(start)] = true;
  stack.push_back(start);
  while(!stack.empty()) {
   const int cell = stack.back();
   stack.pop_back();
   const int cx = cell & 15;
   const int cz = (cell >> 4) & 15;
   const int cy = cell >> 8;
   if(cx == 0)
    faceMask |= 1U << 0;
   if(cx == 15)
    faceMask |= 1U << 1;
   if(cy == 0)
    faceMask |= 1U << 2;
   if(cy == 15)
    faceMask |= 1U << 3;
   if(cz == 0)
    faceMask |= 1U << 4;
   if(cz == 15)
    faceMask |= 1U << 5;
   const auto tryVisit = [&](int next) {
    if(!visited[static_cast<std::size_t>(next)] && !opaque[static_cast<std::size_t>(next)]) {
     visited[static_cast<std::size_t>(next)] = true;
     stack.push_back(next);
    }
   };
   if(cx > 0)
    tryVisit(cell - 1);
   if(cx < 15)
    tryVisit(cell + 1);
   if(cy > 0)
    tryVisit(cell - 256);
   if(cy < 15)
    tryVisit(cell + 256);
   if(cz > 0)
    tryVisit(cell - 16);
   if(cz < 15)
    tryVisit(cell + 16);
  }
  for(int a = 0; a < 6; ++a) {
   if((faceMask & (1U << a)) == 0) {
    continue;
   }
   for(int b = 0; b < 6; ++b) {
    if((faceMask & (1U << b)) != 0) {
     bits |= 1ULL << (a * 6 + b);
    }
   }
  }
 }
 return bits;
}
} // namespace
std::shared_ptr<ChunkMeshJob> ChunkMeshJob::capture(ChunkBuilder& owner,
                                                    client::option::RenderSettings options) {
 // Every bail-out here is permanent from the scheduler's point of view: the
 // section stays dirty and is retried forever, so a silent nullptr wedges all
 // terrain meshing with no symptom but an empty world. Name the reason.
 const auto refuse = [&owner](const char* reason) -> std::shared_ptr<ChunkMeshJob> {
  static int reported = 0;
  if(reported < 8) {
   ++reported;
   char line[192];
   std::snprintf(line, sizeof(line), "[mesh-capture] section (%d,%d,%d) cannot be captured: %s", owner.x,
                 owner.y, owner.z, reason);
   ClientLog::LOGGER.log(::net::minecraft::util::logging::LogLevel::Warning, std::string(line));
  }
  return nullptr;
 };
 net::minecraft::World* world = owner.world;
 if(world == nullptr) {
  return refuse("builder has no world");
 }
 net::minecraft::ChunkSource* source = world->getChunkSource();
 if(source == nullptr) {
  return refuse("world has no chunk source");
 }
 const int minBlockX = owner.x - 1;
 const int minBlockZ = owner.z - 1;
 const int maxBlockX = owner.x + kSectionBlocks + 1;
 const int maxBlockZ = owner.z + kSectionBlocks + 1;
 const int minChunkX = minBlockX >> 4;
 const int minChunkZ = minBlockZ >> 4;
 const int maxChunkX = maxBlockX >> 4;
 const int maxChunkZ = maxBlockZ >> 4;
 std::vector<RegionSnapshot::SourceChunk> sourceChunks;
 sourceChunks.reserve(static_cast<std::size_t>((maxChunkX - minChunkX + 1) * (maxChunkZ - minChunkZ + 1)));
 RenderPinGuard pinGuard(sourceChunks);
 for(int chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
  for(int chunkZ = minChunkZ; chunkZ <= maxChunkZ; ++chunkZ) {
   if(owner.world == nullptr) {
    return refuse("world went away mid-capture");
   }
   if(!source->isChunkLoaded(chunkX, chunkZ)) {
    continue;
   }
    Chunk& chunk = source->getChunk(chunkX, chunkZ);
    if(!chunk.tryAcquireRenderPin()) {
     return refuse("neighbour chunk is evicted");
    }
   sourceChunks.push_back(RegionSnapshot::SourceChunk{chunkX, chunkZ, &chunk});
  }
 }
 // Keep the guard armed until the job fully owns the pins: an exception during
 // job construction (allocation, options copy) or the post-construction shader
 // setup would otherwise leak every acquired pin, which would wedge the chunk
 // eviction path forever. The guard releases them on unwind; once we disarm, the
 // job (whose ~ChunkMeshJob/releasePins releases exactly once) owns them.
 std::array<float, 16> lightLevelToLuminance{};
 std::unique_ptr<net::minecraft::BiomeSource> biomeSource;
 if(owner.world->dimension != nullptr) {
  lightLevelToLuminance = owner.world->dimension->lightLevelToLuminance;
  if(owner.world->dimension->biomeSource) {
   biomeSource = owner.world->dimension->biomeSource->clone();
  }
 } else {
  for(int level = 0; level < 16; ++level) {
   lightLevelToLuminance[static_cast<std::size_t>(level)] = Dimension::luminanceForLightLevel(level);
  }
 }
 auto job = std::shared_ptr<ChunkMeshJob>(new ChunkMeshJob(owner,
                                                           options,
                                                           std::move(sourceChunks),
                                                           owner.world->ambientDarkness,
                                                           lightLevelToLuminance,
                                                           std::move(biomeSource)));
 if(auto* minecraft = net::minecraft::client::Minecraft::INSTANCE;
    minecraft != nullptr && minecraft->gameRenderer != nullptr &&
    minecraft->gameRenderer->shaderPacks() != nullptr) {
  job->blockRenderLayers = minecraft->gameRenderer->packDefinition().blockRenderLayers;
 }
 pinGuard.disarm();
 return job;
}
ChunkMeshJob::ChunkMeshJob(ChunkBuilder& owner,
                           client::option::RenderSettings options,
                           std::vector<RegionSnapshot::SourceChunk> sourceChunks,
                           int ambientDarkness,
                           const std::array<float, 16>& lightLevelToLuminance,
                           std::unique_ptr<net::minecraft::BiomeSource> biomeSource)
    : builder(owner.weak_from_this()),
      version(owner.version),
      x(owner.x),
      y(owner.y),
      z(owner.z),
      opts(options),
      sourceChunks_(std::move(sourceChunks)),
      ambientDarkness_(ambientDarkness),
      lightLevelToLuminance_(lightLevelToLuminance),
      biomeSource_(std::move(biomeSource)) {
}
ChunkMeshJob::~ChunkMeshJob() {
 releasePins();
 // An evicted section is already gone; lock() simply fails and there is no
 // flag left to clear.
 if(const std::shared_ptr<ChunkBuilder> owner = builder.lock()) {
#ifndef NDEBUG
  // The flag itself is main-thread state, so the last shared_ptr<ChunkMeshJob>
  // must still die on the main GL thread (cancelAll/drops stay main-thread-only,
  // WI-4). The marker is latched by GLCore::init() at display setup.
  net::minecraft::util::concurrent::assertOnMainThread();
#endif
  owner->meshJobInFlight = false;
 }
}
void ChunkMeshJob::captureSnapshot() {
 if(snapshot != nullptr) {
  return;
 }
 snapshot = std::make_unique<RegionSnapshot>(sourceChunks_,
                                             ambientDarkness_,
                                             lightLevelToLuminance_,
                                             std::move(biomeSource_),
                                             x - 1,
                                             y - 1,
                                             z - 1,
                                             x + kSectionBlocks + 1,
                                             y + kSectionBlocks,
                                             z + kSectionBlocks + 1);
 releasePins();
}
void ChunkMeshJob::releasePins() noexcept {
 if(pinsReleased_) {
  return;
 }
 for(const RegionSnapshot::SourceChunk& sourceChunk : sourceChunks_) {
  if(sourceChunk.chunk != nullptr) {
   const_cast<Chunk*>(sourceChunk.chunk)->releaseRenderPin();
  }
 }
 pinsReleased_ = true;
}
void ChunkBuilder::buildMesh(ChunkMeshJob& job) {
 // Pins hold the live chunks against eviction. Capture the RegionSnapshot on the
 // worker so the main thread only pays for pin acquisition; memcpy of block/light
 // bands is the expensive part and no longer eats the capture budget.
 if(job.snapshot == nullptr) {
  job.captureSnapshot();
 }
 if(job.snapshot == nullptr) {
  job.failed = true;
  return;
 }
 const int minX = job.x;
 const int minY = job.y;
 const int minZ = job.z;
 const int maxX = job.x + kSectionBlocks;
 const int maxY = job.y + kSectionBlocks;
 const int maxZ = job.z + kSectionBlocks;
 RegionSnapshot& snapshot = *job.snapshot;
 if(!snapshot.columnHasBlocks(job.x, job.z, minY, maxY)) {
  job.result.layerEmpty.fill(true);
  job.result.visibilityBits = ~0ULL;
  job.result.hasSkyLight = snapshot.sawSkyLight();
  return;
 }
 job.result.visibilityBits = computeVisibilityBits(snapshot, minX, minY, minZ);
 Tessellator tessellator;
 tessellator.setCaptureOnly(true);
  block::BlockRenderManager blockRenderManager(tessellator, &snapshot, job.opts);
  ChunkMeshResult& result = job.result;
 for(int layer = 0; layer < terrain_layer::Count; ++layer) {
  bool hasOtherLayer = false;
  bool beganCompile = false;
  bool drewGeometry = false;
  // Mod blocks (texture id >= kModTextureBase) live on their own GL textures,
  // not the terrain atlas, so their faces are captured into a per-texture
  // tessellator here and drawn separately (WorldRenderer::renderModChunkMeshes)
  // with that texture bound. Without this the faces leak into the terrain mesh
  // and sample the atlas with mod UVs -> scrambled/garbled blocks.
  ModMeshCollector modMeshes;
  modMeshes.chunkOffX = -static_cast<double>(job.x);
  modMeshes.chunkOffY = -static_cast<double>(job.y);
  modMeshes.chunkOffZ = -static_cast<double>(job.z);
  blockRenderManager.ctx.modMeshes = &modMeshes;
  const bool interiorFacePass = layer == terrain_layer::CutoutInterior;
  blockRenderManager.ctx.interiorFacePass = interiorFacePass;
  for(int blockX = minX; blockX < maxX; ++blockX) {
   for(int blockZ = minZ; blockZ < maxZ; ++blockZ) {
    for(int blockY = minY; blockY < maxY; ++blockY) {
     const int blockId = snapshot.getBlockId(blockX, blockY, blockZ);
     if(blockId <= 0) {
      continue;
     }
     net::minecraft::block::Block* block =
         net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
     if(block == nullptr) {
      continue;
     }
     if(layer == 0 && Block::BLOCKS_WITH_ENTITY[static_cast<std::size_t>(blockId)]) {
      result.blockEntityPositions.push_back(net::minecraft::Vec3i{blockX, blockY, blockZ});
     }
     const int blockLayer = resolveTerrainMeshLayer(*block, blockId, job.blockRenderLayers);
     if(interiorFacePass) {
      // The interior layer re-runs the leaf blocks the cutout layer already
      // drew, this time emitting only their leaf<->leaf boundaries.
      if(!block::hasLeafInteriorFaces(*block, job.opts.leafInteriorFaces)) {
       continue;
      }
     } else if(blockLayer != layer) {
      hasOtherLayer = true;
      // A leaf block owns geometry in the interior layer as well as its own,
      // so the early-out below must not stop before reaching it.
      continue;
     } else if(block::hasLeafInteriorFaces(*block, job.opts.leafInteriorFaces)) {
      hasOtherLayer = true;
     }
     if(!beganCompile) {
      beganCompile = true;
      tessellator.startQuads();
      // Store vertices section-local: the shader adds chunkOffset
      // (= chunkX - camera) at draw time to reach camera space. No region
      // wrapper, no bake loop on the main thread.
      tessellator.translate(static_cast<double>(-job.x),
                            static_cast<double>(-job.y),
                            static_cast<double>(-job.z));
     }
     drewGeometry |= blockRenderManager.render(*block, blockX, blockY, blockZ);
    }
   }
  }
  if(beganCompile) {
   result.layers[static_cast<std::size_t>(layer)] = tessellator.takeMesh();
  }
  blockRenderManager.ctx.modMeshes = nullptr;
  blockRenderManager.ctx.interiorFacePass = false;
  for(ModMeshCollector::Entry& modEntry : modMeshes.entries) {
   TessellatorMesh modMesh = modEntry.tess.takeMesh();
   if(!modMesh.empty()) {
    result.modLayers[static_cast<std::size_t>(layer)].push_back({modEntry.texture, std::move(modMesh)});
   }
  }
  result.layerEmpty[static_cast<std::size_t>(layer)] =
      !(beganCompile && drewGeometry) && result.modLayers[static_cast<std::size_t>(layer)].empty();
  if(!hasOtherLayer) {
   break;
  }
 }
 result.hasSkyLight = snapshot.sawSkyLight();
}
void ChunkBuilder::uploadMesh(ChunkMeshJob& job) {
 ++chunkUpdates;
 constexpr unsigned kArrayBuffer = 0x8892;
 constexpr unsigned kDynamicDraw = 0x88E8;
 constexpr int kStride = static_cast<int>(sizeof(TessellatorVertex));
 for(int layer = 0; layer < terrain_layer::Count; ++layer) {
  const TessellatorMesh& mesh = job.result.layers[static_cast<std::size_t>(layer)];
  renderLayerEmpty[static_cast<std::size_t>(layer)] = job.result.layerEmpty[static_cast<std::size_t>(layer)];
  LayerVbo& vbo = layerVbos_[static_cast<std::size_t>(layer)];
  if(renderLayerEmpty[static_cast<std::size_t>(layer)] || mesh.empty()) {
   if(vbo.valid()) {
    gl::GLCore::deleteBuffers(1, &vbo.handle);
    vbo = {};
   }
   continue;
  }
  if(!vbo.valid()) {
   gl::GLCore::genBuffers(1, &vbo.handle);
  }
  gl::GLCore::bindBuffer(kArrayBuffer, vbo.handle);
  const auto byteCount = static_cast<std::ptrdiff_t>(mesh.vertices.size() * static_cast<std::size_t>(kStride));
  if(vbo.handle != 0 && gl::GLCore::bufferData != nullptr) {
   // Orphan the old buffer each upload — simpler than sizing and the
   // section mesh is small enough that re-uploading is cheap.
   gl::GLCore::bufferData(kArrayBuffer, byteCount, mesh.vertices.data(), kDynamicDraw);
  }
  vbo.vertexCount = static_cast<int>(mesh.vertices.size());
  gl::GLCore::bindBuffer(kArrayBuffer, 0);
 }
 // Resolve block-entity positions against the live world and apply the same
 // joined/removed diff the old synchronous rebuild kept.
 std::unordered_set<::net::minecraft::block::entity::BlockEntity*> previousBlockEntities;
 previousBlockEntities.insert(blockEntities_.begin(), blockEntities_.end());
 blockEntities_.clear();
 if(world != nullptr) {
  auto& blockEntityDispatcher = block::entity::BlockEntityRenderDispatcher::instance();
  for(const net::minecraft::Vec3i& pos : job.result.blockEntityPositions) {
   ::net::minecraft::block::entity::BlockEntity* blockEntity = world->getBlockEntity(pos.x, pos.y, pos.z);
   if(blockEntity != nullptr && blockEntityDispatcher.hasRenderer(*blockEntity)) {
    blockEntities_.push_back(blockEntity);
   }
  }
 }
 if(currentBlockEntities_ != nullptr) {
  std::unordered_set<::net::minecraft::block::entity::BlockEntity*> currentBlockEntities;
  currentBlockEntities.insert(blockEntities_.begin(), blockEntities_.end());
  for(::net::minecraft::block::entity::BlockEntity* blockEntity : currentBlockEntities) {
   if(!previousBlockEntities.contains(blockEntity)) {
    currentBlockEntities_->push_back(blockEntity);
   }
  }
  for(::net::minecraft::block::entity::BlockEntity* blockEntity : previousBlockEntities) {
   if(!currentBlockEntities.contains(blockEntity)) {
    const auto it = std::find(currentBlockEntities_->begin(), currentBlockEntities_->end(), blockEntity);
    if(it != currentBlockEntities_->end()) {
     currentBlockEntities_->erase(it);
    }
   }
  }
 }
 hasSkyLight = job.result.hasSkyLight;
 occlusion.visBits = job.result.visibilityBits;
 freeModMeshGpuBuffers();
 modLayerMeshes_ = std::move(job.result.modLayers);
 for(int layer = 0; layer < terrain_layer::Count; ++layer) {
  for(ModChunkMesh& modMesh : modLayerMeshes_[static_cast<std::size_t>(layer)]) {
   (void)modMesh.mesh.uploadToGpu();
  }
 }
 built = true;
}
void ChunkBuilder::drawLayer(int layer) const {
 const LayerVbo& vbo = layerVbos_[static_cast<std::size_t>(layer)];
 if(!vbo.valid() || renderLayerEmpty[static_cast<std::size_t>(layer)]) {
  return;
 }
 if(!quad_index::ensure(static_cast<std::size_t>(vbo.vertexCount))) {
  return;
 }
 constexpr unsigned kArrayBuffer = 0x8892;
 constexpr int kStride = static_cast<int>(sizeof(TessellatorVertex));
 render::core::RenderPass pass;
 pass.modelView = render::core::drawModelView();
 pass.projection = render::core::drawProjection();
 pass.fog = render::core::fog();
 // Section-local vertices: this is what carries the section's world position
 // into the shader. WorldRenderer published it via setPendingTerrainDraw.
 render::core::applyPendingTerrain(pass);
 pass.buffer = vbo.handle;
 pass.vertexCount = static_cast<std::size_t>(vbo.vertexCount);
 pass.stride = kStride;
 pass.hasTexture = true;
 pass.hasColor = true;
 pass.hasNormals = true;
 const int indexCount = (vbo.vertexCount / 4) * 6;
 render::core::submitIndexedQuads(pass, quad_index::handle(), indexCount);
 gl::GLCore::bindBuffer(kArrayBuffer, 0);
 ++frameDrawCalls;
}
void ChunkBuilder::freeGpuBuffers() noexcept {
 for(int layer = 0; layer < terrain_layer::Count; ++layer) {
  if(layerVbos_[static_cast<std::size_t>(layer)].valid()) {
   gl::GLCore::deleteBuffers(1, &layerVbos_[static_cast<std::size_t>(layer)].handle);
   layerVbos_[static_cast<std::size_t>(layer)] = {};
  }
 }
 freeModMeshGpuBuffers();
}
} // namespace net::minecraft::client::render::chunk
