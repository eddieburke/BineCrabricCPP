#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include <algorithm>
#include <array>
#include <unordered_set>
namespace net::minecraft::client::render::chunk {
namespace {
class RenderPinGuard {
public:
  explicit RenderPinGuard(std::vector<RegionSnapshot::SourceChunk>& pinned) noexcept : pinned_(pinned) {}
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
// Seam-closing scale baked into VBO vertices at upload time.
constexpr float kChunkVertexScale = 1.000001f;
// Fold the per-chunk transform into the vertices so a whole region shares one
// coordinate space and draws with a single outer translate.
void bakeRegionVertices(TessellatorMesh& mesh, int renderX, int renderY, int renderZ, int sizeY, int sizeZ) {
  constexpr float scale = kChunkVertexScale;
  const float addX = (static_cast<float>(sizeZ) / 2.0f) * (scale - 1.0f) + static_cast<float>(renderX);
  const float addY = (static_cast<float>(sizeY) / 2.0f) * (scale - 1.0f) + static_cast<float>(renderY);
  const float addZ = (static_cast<float>(sizeZ) / 2.0f) * (scale - 1.0f) + static_cast<float>(renderZ);
  for(TessellatorVertex& vertex : mesh.vertices) {
    vertex.x = scale * vertex.x + addX;
    vertex.y = scale * vertex.y + addY;
    vertex.z = scale * vertex.z + addZ;
  }
}
} // namespace
std::shared_ptr<ChunkMeshJob> ChunkMeshJob::capture(
    ChunkBuilder& owner, client::option::ResolvedRenderOptions options, bool fancyGraphics) {
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
      if(owner.world == nullptr || !source->isChunkLoaded(chunkX, chunkZ)) {
        return nullptr;
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
  return std::shared_ptr<ChunkMeshJob>(new ChunkMeshJob(owner, options, fancyGraphics, std::move(sourceChunks),
                                                        owner.world->ambientDarkness, lightLevelToLuminance, std::move(biomeSource)));
}
ChunkMeshJob::ChunkMeshJob(ChunkBuilder& owner, client::option::ResolvedRenderOptions options, bool fancyGraphicsIn,
                           std::vector<RegionSnapshot::SourceChunk> sourceChunks, int ambientDarkness,
                           const std::array<float, 16>& lightLevelToLuminance, std::unique_ptr<net::minecraft::BiomeSource> biomeSource)
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
}
void ChunkMeshJob::captureSnapshot() {
  if(snapshot != nullptr) {
    return;
  }
  snapshot = std::make_unique<RegionSnapshot>(sourceChunks_, ambientDarkness_, lightLevelToLuminance_,
                                              std::move(biomeSource_), x - 1, y - 1, z - 1, x + sizeX + 1, y + sizeY, z + sizeZ + 1);
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
  Tessellator tessellator;
  tessellator.setCaptureOnly(true);
  RegionSnapshot& snapshot = *job.snapshot;
  block::BlockRenderManager blockRenderManager(&snapshot, job.opts, job.fancyGraphics);
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
            tessellator.translate(
                static_cast<double>(-job.x), static_cast<double>(-job.y), static_cast<double>(-job.z));
          }
          drewGeometry |= blockRenderManager.render(*block, blockX, blockY, blockZ);
        }
      }
    }
    if(beganCompile) {
      result.layers[static_cast<std::size_t>(layer)] = tessellator.takeMesh();
      bakeRegionVertices(result.layers[static_cast<std::size_t>(layer)], job.renderX, job.renderY, job.renderZ,
                         job.sizeY, job.sizeZ);
    }
    blockRenderManager.ctx.modMeshes = nullptr;
    for(ModMeshCollector::Entry& modEntry : modMeshes.entries) {
      TessellatorMesh modMesh = modEntry.tess.takeMesh();
      if(!modMesh.empty()) {
        result.modLayers[static_cast<std::size_t>(layer)].push_back({modEntry.texture, std::move(modMesh)});
      }
    }
    result.layerEmpty[static_cast<std::size_t>(layer)] = !(beganCompile && drewGeometry);
    if(!hasOtherLayer) {
      break;
    }
  }
  result.hasSkyLight = snapshot.sawSkyLight();
}
void ChunkBuilder::uploadMesh(const ChunkMeshJob& job) {
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
    buffer.upload(slot, mesh.vertices.data(), static_cast<int>(mesh.vertices.size()), mesh.hasTexture, mesh.hasColor,
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
  modLayerMeshes_[0] = job.result.modLayers[0];
  modLayerMeshes_[1] = job.result.modLayers[1];
  built = true;
}
} // namespace net::minecraft::client::render::chunk
