#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include <algorithm>
#include <array>
#include <unordered_set>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
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
 for(int y = 0; y < 16; ++y) {
  for(int z = 0; z < 16; ++z) {
   for(int x = 0; x < 16; ++x) {
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
                                                    client::option::ResolvedRenderOptions options,
                                                    bool fancyGraphics) {
 net::minecraft::World* world = owner.world;
 if(world == nullptr) {
  return nullptr;
 }
 net::minecraft::ChunkSource* source = world->getChunkSource();
 if(source == nullptr) {
  return nullptr;
 }
 const int minBlockX = owner.x - 1;
 const int minBlockZ = owner.z - 1;
 const int maxBlockX = owner.x + owner.sizeX + 1;
 const int maxBlockZ = owner.z + owner.sizeZ + 1;
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
    return nullptr;
   }
   if(!source->isChunkLoaded(chunkX, chunkZ)) {
    continue;
   }
   Chunk& chunk = source->getChunk(chunkX, chunkZ);
   if(!chunk.tryAcquireRenderPin()) {
    return nullptr;
   }
   sourceChunks.push_back(RegionSnapshot::SourceChunk{chunkX, chunkZ, &chunk});
  }
 }
 pinGuard.disarm();
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
 return std::shared_ptr<ChunkMeshJob>(new ChunkMeshJob(owner,
                                                       options,
                                                       fancyGraphics,
                                                       std::move(sourceChunks),
                                                       owner.world->ambientDarkness,
                                                       lightLevelToLuminance,
                                                       std::move(biomeSource)));
}
ChunkMeshJob::ChunkMeshJob(ChunkBuilder& owner,
                           client::option::ResolvedRenderOptions options,
                           bool fancyGraphicsIn,
                           std::vector<RegionSnapshot::SourceChunk> sourceChunks,
                           int ambientDarkness,
                           const std::array<float, 16>& lightLevelToLuminance,
                           std::unique_ptr<net::minecraft::BiomeSource> biomeSource)
    : builder(&owner),
      version(owner.version),
      x(owner.x),
      y(owner.y),
      z(owner.z),
      sizeX(owner.sizeX),
      sizeY(owner.sizeY),
      sizeZ(owner.sizeZ),
      renderX(owner.renderX),
      renderY(owner.renderY),
      renderZ(owner.renderZ),
      opts(options),
      fancyGraphics(fancyGraphicsIn),
      sourceChunks_(std::move(sourceChunks)),
      ambientDarkness_(ambientDarkness),
      lightLevelToLuminance_(lightLevelToLuminance),
      biomeSource_(std::move(biomeSource)) {
}
ChunkMeshJob::~ChunkMeshJob() {
 releasePins();
 if(builder != nullptr) {
  builder->meshJobInFlight = false;
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
                                             x + sizeX + 1,
                                             y + sizeY,
                                             z + sizeZ + 1);
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
 // The snapshot is captured on the main thread before dispatch (WorldRendererCore
 // enqueueMeshJob) so the memcpy of live chunk blocks/light cannot race main-thread
 // writes. Do NOT capture here on the worker. A null snapshot means a buggy enqueue
 // path: fail so the job reschedules and gets captured on main next frame.
 if(job.snapshot == nullptr) {
  job.failed = true;
  return;
 }
 const int minX = job.x;
 const int minY = job.y;
 const int minZ = job.z;
 const int maxX = job.x + job.sizeX;
 const int maxY = job.y + job.sizeY;
 const int maxZ = job.z + job.sizeZ;
 RegionSnapshot& snapshot = *job.snapshot;
 if(!snapshot.columnHasBlocks(job.x, job.z, minY, maxY)) {
  job.result.layerEmpty = {true, true};
  job.result.visibilityBits = ~0ULL;
  job.result.hasSkyLight = snapshot.sawSkyLight();
  return;
 }
 job.result.visibilityBits = computeVisibilityBits(snapshot, minX, minY, minZ);
 Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 block::BlockRenderManager blockRenderManager(&snapshot, job.opts);
 blockRenderManager.ctx.tess = &tessellator;
 ChunkMeshResult& result = job.result;
 for(int layer = 0; layer < 2; ++layer) {
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
  for(int blockY = minY; blockY < maxY; ++blockY) {
   for(int blockZ = minZ; blockZ < maxZ; ++blockZ) {
    for(int blockX = minX; blockX < maxX; ++blockX) {
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
     const int blockLayer = block->getRenderLayer();
     if(blockLayer != layer) {
      hasOtherLayer = true;
      continue;
     }
     if(!beganCompile) {
      beganCompile = true;
      tessellator.startQuads();
      // Region-space bake folded into the capture translate: vertices land at
      // renderX-relative coordinates directly, replacing the old post-pass.
      tessellator.translate(static_cast<double>(job.renderX - job.x),
                            static_cast<double>(job.renderY - job.y),
                            static_cast<double>(job.renderZ - job.z));
     }
     drewGeometry |= blockRenderManager.render(*block, blockX, blockY, blockZ);
    }
   }
  }
  if(beganCompile) {
   result.layers[static_cast<std::size_t>(layer)] = tessellator.takeMesh();
  }
  blockRenderManager.ctx.modMeshes = nullptr;
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
 if(region_ == nullptr) {
  region_ = &regionManager_->regionFor(cameraOffsetX, cameraOffsetY, cameraOffsetZ);
 }
 for(int layer = 0; layer < 2; ++layer) {
  const TessellatorMesh& mesh = job.result.layers[static_cast<std::size_t>(layer)];
  renderLayerEmpty[static_cast<std::size_t>(layer)] = job.result.layerEmpty[static_cast<std::size_t>(layer)];
  ChunkRegionBuffer& buffer = region_->layers[static_cast<std::size_t>(layer)];
  ChunkRegionBuffer::Slot& slot = regionSlots_[static_cast<std::size_t>(layer)];
  if(renderLayerEmpty[static_cast<std::size_t>(layer)] || mesh.empty()) {
   buffer.release(slot);
   continue;
  }
  buffer.upload(slot,
                mesh.vertices.data(),
                static_cast<int>(mesh.vertices.size()),
                mesh.hasTexture,
                mesh.hasColor,
                mesh.hasNormals);
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
 visBits = job.result.visibilityBits;
 freeModMeshGpuBuffers();
 modLayerMeshes_[0] = std::move(job.result.modLayers[0]);
 modLayerMeshes_[1] = std::move(job.result.modLayers[1]);
 for(int layer = 0; layer < 2; ++layer) {
  for(ModChunkMesh& modMesh : modLayerMeshes_[static_cast<std::size_t>(layer)]) {
   modMesh.mesh.uploadToGpu();
  }
 }
 built = true;
}
} // namespace net::minecraft::client::render::chunk
