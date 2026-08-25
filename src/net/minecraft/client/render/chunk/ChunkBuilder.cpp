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
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/util/concurrent/ThreadNames.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
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

 private:
 void release() noexcept {
  for(const RegionSnapshot::SourceChunk& sourceChunk : pinned_) {
   if(sourceChunk.chunk != nullptr) {
    const_cast<Chunk*>(sourceChunk.chunk)->releaseRenderPin();
   }
  }
  pinned_.clear();
 }
 std::vector<RegionSnapshot::SourceChunk>& pinned_;
};
// Flood-fills the section's non-opaque cells and records which face pairs a
// component connects (bit a*6+b). Faces: 0:-X 1:+X 2:-Y 3:+Y 4:-Z 5:+Z.
std::uint64_t computeVisibilityBits(const RegionSnapshot& snapshot, int minX, int minY, int minZ) {
 // Column pointers and the baked opacity table, for the same reason the mesh loop
 // uses them: 4,096 accessor calls and a per-id virtual isOpaque() cache, for an
 // answer that is one byte load and one table lookup.
 std::array<bool, 4096> opaque{};
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   const RegionSnapshot::ColumnNeighbourhood columns = snapshot.columnNeighbourhood(minX + x, minZ + z);
   if(columns.self == nullptr) {
    continue;
   }
   const int base = columns.rowFor(minY);
   for(int y = 0; y < 16; ++y) {
    opaque[static_cast<std::size_t>((y << 8) | (z << 4) | x)] =
        net::minecraft::block::Block::BLOCKS_OPAQUE[columns.self[base + y]];
   }
  }
 }
 std::array<bool, 4096> visited{};
 std::uint64_t bits = 0;
 // One flood fill per section, several sections a frame: the reserve was a heap
 // allocation and a free every time.
 static thread_local std::vector<int> stack;
 stack.clear();
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
 auto snapshot = std::make_unique<RegionSnapshot>(sourceChunks,
                                                  owner.world->ambientDarkness,
                                                  lightLevelToLuminance,
                                                  std::move(biomeSource),
                                                  owner.x - 1,
                                                  owner.y - 1,
                                                  owner.z - 1,
                                                  owner.x + kSectionBlocks + 1,
                                                  owner.y + kSectionBlocks,
                                                  owner.z + kSectionBlocks + 1);
 auto job = std::shared_ptr<ChunkMeshJob>(new ChunkMeshJob(owner, options, std::move(snapshot)));
 if(auto* minecraft = net::minecraft::client::Minecraft::INSTANCE;
    minecraft != nullptr && minecraft->gameRenderer != nullptr &&
    minecraft->gameRenderer->shaderPipeline() != nullptr) {
  job->blockRenderLayers = minecraft->gameRenderer->packDefinition().blockRenderLayers;
 }
 return job;
}
ChunkMeshJob::ChunkMeshJob(ChunkBuilder& owner,
                           client::option::RenderSettings options,
                           std::unique_ptr<RegionSnapshot> capturedSnapshot)
    : builder(owner.weak_from_this()),
      version(owner.version),
      x(owner.x),
      y(owner.y),
      z(owner.z),
      snapshot(std::move(capturedSnapshot)),
      opts(options) {
}
ChunkMeshJob::~ChunkMeshJob() {
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
void ChunkBuilder::buildMesh(ChunkMeshJob& job) {
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
 // One per mesh worker, not one per job. A fresh Tessellator starts with a 4 KB
 // buffer and grows it to the section's real size through vector::insert, so every
 // job paid the whole realloc ladder and then freed the result -- the malloc_base
 // cost the look-around profile attributed to emitBlockVertex. Reused, the buffer
 // reaches its high-water mark once and every later job appends into it. start()
 // resets all of the per-part state below.
 static thread_local Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 block::BlockRenderManager blockRenderManager(tessellator, &snapshot, job.opts);
 ChunkMeshResult& result = job.result;
 const TerrainLayerTable layerOf = buildTerrainLayerTable(job.blockRenderLayers);
 // One walk over the section, bucketing each block into the layer that will draw it,
 // instead of walking all 4096 once per layer. The old loop re-read every column,
 // re-tested every enclosure and re-resolved every layer up to four times, and needed
 // a hasOtherLayer flag to decide when it could stop early. Positions pack into the
 // low twelve bits; the buckets live on the worker so no job allocates them.
 static thread_local std::array<std::vector<std::uint16_t>, terrain_layer::Count> buckets;
 for(std::vector<std::uint16_t>& bucket : buckets) {
  bucket.clear();
 }
 for(int blockX = minX; blockX < maxX; ++blockX) {
  for(int blockZ = minZ; blockZ < maxZ; ++blockZ) {
   // One chunk lookup per column instead of one per voxel, and the block ids come
   // straight off the snapshot's own storage. A column with no self pointer means
   // the section's chunk is absent from the snapshot, which is the same thing as
   // every block in it reading back as air.
   const RegionSnapshot::ColumnNeighbourhood columns = snapshot.columnNeighbourhood(blockX, blockZ);
   if(columns.self == nullptr) {
    continue;
   }
   for(int blockY = minY; blockY < maxY; ++blockY) {
    const int row = columns.rowFor(blockY);
    const int blockId = columns.self[row];
    if(blockId <= 0) {
     continue;
    }
    net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
    if(block == nullptr) {
     continue;
    }
    if(Block::BLOCKS_WITH_ENTITY[static_cast<std::size_t>(blockId)]) {
     result.blockEntityPositions.push_back(net::minecraft::Vec3i{blockX, blockY, blockZ});
    }
    if(columns.fullyEnclosed(row)) {
     continue;
    }
    const auto packed = static_cast<std::uint16_t>((((blockX - minX) & 0xF) << 8) |
                                                   (((blockY - minY) & 0xF) << 4) | ((blockZ - minZ) & 0xF));
    buckets[static_cast<std::size_t>(layerOf[static_cast<std::size_t>(blockId)])].push_back(packed);
    // A leaf block owns geometry in the interior layer as well as its own.
    if(block::hasLeafInteriorFaces(*block, job.opts.leafInteriorFaces)) {
     buckets[terrain_layer::CutoutInterior].push_back(packed);
    }
   }
  }
 }
 for(int layer = 0; layer < terrain_layer::Count; ++layer) {
  bool beganCompile = false;
  bool drewGeometry = false;
  ModMeshCollector modMeshes;
  modMeshes.chunkOffX = -static_cast<double>(job.x);
  modMeshes.chunkOffY = -static_cast<double>(job.y);
  modMeshes.chunkOffZ = -static_cast<double>(job.z);
  blockRenderManager.ctx.modMeshes = &modMeshes;
  blockRenderManager.ctx.interiorFacePass = layer == terrain_layer::CutoutInterior;
  for(const std::uint16_t packed : buckets[static_cast<std::size_t>(layer)]) {
   const int blockX = minX + ((packed >> 8) & 0xF);
   const int blockY = minY + ((packed >> 4) & 0xF);
   const int blockZ = minZ + (packed & 0xF);
   net::minecraft::block::Block* block =
       net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(snapshot.getBlockId(blockX, blockY, blockZ))];
   if(block == nullptr) {
    continue;
   }
   if(!beganCompile) {
    beganCompile = true;
    tessellator.startQuads();
    constexpr int kRegionBlocksX = 8 * kSectionBlocks;
    constexpr int kRegionBlocksY = 4 * kSectionBlocks;
    constexpr int kRegionBlocksZ = 8 * kSectionBlocks;
    tessellator.translate(
        static_cast<double>(-net::minecraft::util::math::MathHelper::floorDiv(job.x, kRegionBlocksX) *
                            kRegionBlocksX),
        static_cast<double>(-net::minecraft::util::math::MathHelper::floorDiv(job.y, kRegionBlocksY) *
                            kRegionBlocksY),
        static_cast<double>(-net::minecraft::util::math::MathHelper::floorDiv(job.z, kRegionBlocksZ) *
                            kRegionBlocksZ));
   }
   drewGeometry |= blockRenderManager.render(*block, blockX, blockY, blockZ);
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
 }
 result.hasSkyLight = snapshot.sawSkyLight();
}
void ChunkBuilder::uploadMesh(ChunkMeshJob& job) {
 ++chunkUpdates;
 for(int layer = 0; layer < terrain_layer::Count; ++layer) {
  TessellatorMesh& mesh = job.result.layers[static_cast<std::size_t>(layer)];
  renderLayerEmpty[static_cast<std::size_t>(layer)] = job.result.layerEmpty[static_cast<std::size_t>(layer)];
  TerrainAllocation& allocation = terrainAllocations_[static_cast<std::size_t>(layer)];
  if(mesh.empty() || terrainRegion_ == nullptr) {
   if(terrainRegion_ != nullptr) terrainRegion_->release(layer, allocation);
  } else if(!terrainRegion_->upload(layer, allocation, mesh.vertices)) {
   terrainRegion_->release(layer, allocation);
   renderLayerEmpty[static_cast<std::size_t>(layer)] =
       job.result.modLayers[static_cast<std::size_t>(layer)].empty();
  }
  // The arena owns the vertices now; hand the buffer back rather than letting the job
  // destructor free it on this thread.
  mesh.releaseCpuVertices();
 }
 // Resolve block-entity positions against the live world and apply the same
 // joined/removed diff the old synchronous rebuild kept.
 const bool hadBlockEntities = !blockEntities_.empty();
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
 if(currentBlockEntities_ != nullptr && (hadBlockEntities || !blockEntities_.empty())) {
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
  auto& meshes = modLayerMeshes_[static_cast<std::size_t>(layer)];
  std::sort(meshes.begin(), meshes.end(), [](const ModChunkMesh& a, const ModChunkMesh& b) {
   return a.texture < b.texture;
  });
  for(ModChunkMesh& modMesh : meshes) {
   if(modMesh.mesh.uploadToGpu()) modMesh.mesh.releaseCpuVertices();
  }
 }
 built = true;
}
void ChunkBuilder::freeGpuBuffers() noexcept {
 for(int layer = 0; layer < terrain_layer::Count; ++layer) {
  if(terrainRegion_ != nullptr) {
   terrainRegion_->release(layer, terrainAllocations_[static_cast<std::size_t>(layer)]);
  } else {
   terrainAllocations_[static_cast<std::size_t>(layer)] = {};
  }
 }
 freeModMeshGpuBuffers();
}
} // namespace net::minecraft::client::render::chunk
