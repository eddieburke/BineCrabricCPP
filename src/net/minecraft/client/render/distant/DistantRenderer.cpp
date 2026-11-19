#include "net/minecraft/client/render/distant/DistantRenderer.hpp"
#include "net/minecraft/client/render/distant/NetherTile.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
namespace net::minecraft::client::render::distant {
namespace {
constexpr int kRetryFrames = 30;
constexpr std::size_t kSpareMeshes = 8;
void normalMatrix(const util::math::Matrix4f& modelView, float out[9]) {
 util::math::Matrix4f inverse = modelView;
 inverse.invert();
 for(int column = 0; column < 3; ++column) {
  for(int row = 0; row < 3; ++row) {
   out[column * 3 + row] = inverse.m[row * 4 + column];
  }
 }
}
} // namespace
void DistantRenderer::PinnedChunks::release() noexcept {
 for(Chunk*& chunk : chunks) {
  if(chunk != nullptr) {
   chunk->releaseRenderPin();
   chunk = nullptr;
  }
 }
}
DistantRenderer::DistantRenderer() : palette_(std::make_shared<const BlockPalette>(defaultPalette())) {
}
DistantRenderer::~DistantRenderer() {
 reset();
 slab_.destroy();
}
void DistantRenderer::releaseTile(Tile& tile) {
 if(tile.cancellation != nullptr) tile.cancellation->store(true, std::memory_order_release);
 if(tile.opaque.has_value()) slab_.release(*tile.opaque);
 if(tile.water.has_value()) slab_.release(*tile.water);
 if(tile.ice.has_value()) slab_.release(*tile.ice);
 tile.opaque.reset();
 tile.water.reset();
 tile.ice.reset();
}
void DistantRenderer::reset() {
 shared_->cancelled.store(true, std::memory_order_release);
 if(!poolLifetime_.expired()) pool_->cancelPending(this);
 ++coverageRevision;
 for(auto& [key, tile] : tiles_) {
  releaseTile(tile);
 }
 tiles_.clear();
 wanted_.clear();
 buildCandidates_.clear();
 grid_.reset();
 shared_ = std::make_shared<Shared>();
 biomes_.reset();
 world_ = nullptr;
 worldSeed_ = 0;
 netherGeometry_ = false;
 inFlight_ = 0;
 memoryStride_ = 1;
 nativeRadius_ = -1;
 clipDistance_ = 0.0;
 transitionCoverage_.fill(0);
 coverageDirty_ = true;
 ambientOcclusion_.reset();
 drawnLastFrame_ = 0;
}
void DistantRenderer::updateNativeRadius(const ChunkSectionSystem& sections, int requestedRadius) {
 if(nativeRadius_ < 0 || requestedRadius <= nativeRadius_ || sections.nativeRadiusReady(requestedRadius))
  nativeRadius_ = requestedRadius;
 clipDistance_ = static_cast<double>(std::max(0, nativeRadius_ * 16 - kNativeOverlapBlocks));
}
int DistantRenderer::meshPriority(int distance, bool replacement) {
 return 8 + std::max(0, distance) / 16 + (replacement ? 64 : 0);
}
void DistantRenderer::collectBuildCandidates(std::size_t limit) {
 buildCandidates_.clear();
 for(const auto& [distance, key] : wanted_) {
  const Tile& tile = tiles_.at(key);
  if(tile.inFlight || tile.retryFrame > static_cast<int>(frame_) ||
     (tile.builtGeneration == tile.generation && tile.stride == tile.wantedStride)) continue;
  const bool reclaimingMemory = tile.stride != 0 && tile.wantedStride > tile.stride;
  const int priority = reclaimingMemory ? 0 : meshPriority(distance, tile.chunkMask == 0xFFFFu && tile.readyPinMask == 0);
  buildCandidates_.emplace_back(priority, key);
 }
 const auto compare = [](const auto& a, const auto& b) {
  if(a.first != b.first) return a.first < b.first;
  if(a.second.x != b.second.x) return a.second.x < b.second.x;
  return a.second.z < b.second.z;
 };
 if(limit > 0 && limit < buildCandidates_.size()) {
  std::partial_sort(buildCandidates_.begin(), buildCandidates_.begin() + static_cast<std::ptrdiff_t>(limit),
                     buildCandidates_.end(), compare);
  buildCandidates_.resize(limit);
 } else {
  std::sort(buildCandidates_.begin(), buildCandidates_.end(), compare);
 }
}
void DistantRenderer::markChunkDirty(int chunkX, int chunkZ, bool discardPending) {
 const int tileX = MathHelper::floorDiv(chunkX, kTileChunks);
 const int tileZ = MathHelper::floorDiv(chunkZ, kTileChunks);
 const int localX = chunkX - tileX * kTileChunks;
 const int localZ = chunkZ - tileZ * kTileChunks;
 for(int dz = localZ == 0 ? -1 : 0; dz <= (localZ == kTileChunks - 1 ? 1 : 0); ++dz) {
  for(int dx = localX == 0 ? -1 : 0; dx <= (localX == kTileChunks - 1 ? 1 : 0); ++dx) {
   const auto it = tiles_.find(TileKey{tileX + dx, tileZ + dz});
   if(it != tiles_.end()) {
    ++it->second.generation;
    if(discardPending) it->second.minimumGeneration = it->second.generation;
    it->second.retryFrame = 0;
   }
  }
 }
}
void DistantRenderer::drainCompleted(std::size_t uploadBudget) {
 std::vector<Completed> completed;
 {
  const std::lock_guard lock(shared_->mutex);
  completed.swap(shared_->completed);
 }
 std::size_t uploaded = 0;
 bool memoryPressure = false;
 std::vector<Completed> deferred;
 for(Completed& result : completed) {
  const auto it = tiles_.find(result.key);
  if(it == tiles_.end()) {
   --inFlight_;
   continue;
  }
  Tile& tile = it->second;
  if(result.jobId != tile.jobId) {
   --inFlight_;
   continue;
  }
  if(result.chunkMask == 0) {
   --inFlight_;
   tile.inFlight = false;
   tile.cancellation.reset();
   tile.retryFrame = static_cast<int>(frame_) + kRetryFrames;
   continue;
  }
  const bool coverageProgress = result.chunkMask != tile.chunkMask &&
                               (result.chunkMask & tile.chunkMask) == tile.chunkMask &&
                               result.generation < tile.generation;
  const bool generationProgress = result.generation > tile.builtGeneration &&
                                  (result.chunkMask & tile.chunkMask) == tile.chunkMask &&
                                  result.generation < tile.generation;
  const bool strideAcceptable = result.stride == tile.wantedStride ||
                                (tile.stride == 0 && result.stride > 0 && coverageProgress);
  if(result.generation < tile.minimumGeneration ||
     (result.generation != tile.generation && !coverageProgress && !generationProgress) || !strideAcceptable) {
   --inFlight_;
   tile.inFlight = false;
   tile.cancellation.reset();
   continue;
  }
  const std::size_t bytes = (result.mesh.opaque.size() + result.mesh.water.size() + result.mesh.ice.size()) * sizeof(DhVertex);
  if(uploaded > 0 && uploaded + bytes > uploadBudget) {
   deferred.push_back(std::move(result));
   continue;
  }
  --inFlight_;
  tile.inFlight = false;
  tile.cancellation.reset();
  std::optional<DistantSlab::Allocation> opaque;
  std::optional<DistantSlab::Allocation> water;
  std::optional<DistantSlab::Allocation> ice;
  bool reuseOpaque = false;
  bool reuseWater = false;
  bool reuseIce = false;
  const auto reserve = [this](const std::optional<DistantSlab::Allocation>& old, std::size_t vertices, bool& reuse)
      -> std::optional<DistantSlab::Allocation> {
   if(vertices == 0) return std::nullopt;
   const auto pages = static_cast<std::uint32_t>((vertices + DistantSlab::kPageVertices - 1) / DistantSlab::kPageVertices);
   if(old.has_value() && pages <= old->pageCount) {
    reuse = true;
    return DistantSlab::Allocation{old->firstPage, pages, static_cast<std::uint32_t>(vertices)};
   }
   return slab_.allocate(vertices);
  };
  opaque = reserve(tile.opaque, result.mesh.opaque.size(), reuseOpaque);
  water = reserve(tile.water, result.mesh.water.size(), reuseWater);
  ice = reserve(tile.ice, result.mesh.ice.size(), reuseIce);
  if((!result.mesh.opaque.empty() && !opaque.has_value()) || (!result.mesh.water.empty() && !water.has_value()) ||
     (!result.mesh.ice.empty() && !ice.has_value())) {
   if(opaque.has_value() && !reuseOpaque) slab_.release(*opaque);
   if(water.has_value() && !reuseWater) slab_.release(*water);
   if(ice.has_value() && !reuseIce) slab_.release(*ice);
   memoryPressure = true;
   tile.retryFrame = static_cast<int>(frame_) + kRetryFrames;
   continue;
  }
  const auto retire = [this](const std::optional<DistantSlab::Allocation>& old,
                             const std::optional<DistantSlab::Allocation>& replacement, bool reuse) {
   if(!old.has_value()) return;
   if(!reuse) {
    slab_.release(*old);
   } else if(old->pageCount > replacement->pageCount) {
    slab_.release(DistantSlab::Allocation{old->firstPage + replacement->pageCount,
                                          old->pageCount - replacement->pageCount, 0});
   }
  };
  retire(tile.opaque, opaque, reuseOpaque);
  retire(tile.water, water, reuseWater);
  retire(tile.ice, ice, reuseIce);
  if(opaque.has_value()) slab_.upload(*opaque, result.mesh.opaque);
  if(water.has_value()) slab_.upload(*water, result.mesh.water);
  if(ice.has_value()) slab_.upload(*ice, result.mesh.ice);
  tile.opaque = opaque;
  tile.water = water;
  tile.ice = ice;
  tile.minY = result.mesh.minY;
  tile.maxY = result.mesh.maxY;
  tile.stride = result.stride;
  if(tile.chunkMask != result.chunkMask) ++coverageRevision;
  tile.chunkMask = result.chunkMask;
  tile.builtPinMask = result.pinMask;
  tile.builtGeneration = result.generation;
  uploaded += bytes;
 }
 const bool reclaiming = memoryPressure && std::any_of(tiles_.begin(), tiles_.end(), [](const auto& entry) {
  return entry.second.stride != 0 && entry.second.wantedStride > entry.second.stride;
 });
 if(memoryPressure && !reclaiming) {
  memoryStride_ = std::min(kMaxStride, memoryStride_ * 2);
 } else if(!memoryPressure && memoryStride_ > 1 && inFlight_ == 0 && buildCandidates_.empty() &&
           slab_.usedBytes() * 4 < slab_.capacityBytes()) {
  memoryStride_ /= 2;
 }
 if(completed.empty()) return;
 const std::lock_guard lock(shared_->mutex);
 for(Completed& result : deferred) shared_->completed.push_back(std::move(result));
 for(Completed& result : completed) {
  if(shared_->spareMeshes.size() >= kSpareMeshes) break;
  if(result.mesh.opaque.capacity() == 0 && result.mesh.water.capacity() == 0 && result.mesh.ice.capacity() == 0) continue;
  result.mesh.opaque.clear();
  result.mesh.water.clear();
  result.mesh.ice.clear();
  shared_->spareMeshes.push_back(std::move(result.mesh));
 }
}
void DistantRenderer::updateTileGrid(const ChunkSectionSystem& sections, const option::RenderSettings& settings) {
 const GridState state{MathHelper::floor(eyeX_) >> 4, MathHelper::floor(eyeZ_) >> 4,
                       settings.renderDistance.blocks, nativeRadius_, settings.nativeChunkRadius,
                       settings.lodQuality, memoryStride_};
 if(grid_.has_value() && *grid_ == state) return;
 grid_ = state;
 constexpr double margin = 11.313708498984761;
 const double gridX = static_cast<double>(state.cameraX) * 16.0 + 8.0;
 const double gridZ = static_cast<double>(state.cameraZ) * 16.0 + 8.0;
 const double viewBlocks = settings.renderDistance.blocks;
 const double nativeBlocks = static_cast<double>(settings.nativeChunkRadius * 16);
 const double innerBlocks = std::max(0, nativeRadius_ * 16 - kNativeOverlapBlocks);
 const int reach = static_cast<int>(std::ceil((viewBlocks + margin) / kTileBlocks)) + 1;
 const int centerX = MathHelper::floorDiv(state.cameraX, kTileChunks);
 const int centerZ = MathHelper::floorDiv(state.cameraZ, kTileChunks);
 wanted_.clear();
 for(int tz = centerZ - reach; tz <= centerZ + reach; ++tz) {
  for(int tx = centerX - reach; tx <= centerX + reach; ++tx) {
   const double minX = static_cast<double>(tx) * kTileBlocks;
   const double minZ = static_cast<double>(tz) * kTileBlocks;
   const double nearX = std::clamp(gridX, minX, minX + kTileBlocks) - gridX;
   const double nearZ = std::clamp(gridZ, minZ, minZ + kTileBlocks) - gridZ;
   const double nearest = std::max(0.0, std::sqrt(nearX * nearX + nearZ * nearZ) - margin);
   if(nearest > viewBlocks) continue;
   const double farX = std::max(std::abs(minX - gridX), std::abs(minX + kTileBlocks - gridX));
   const double farZ = std::max(std::abs(minZ - gridZ), std::abs(minZ + kTileBlocks - gridZ));
   const double farthest = std::sqrt(farX * farX + farZ * farZ) + margin;
   bool holdingForNative = false;
   const auto existing = tiles_.find(TileKey{tx, tz});
   if(nearest < innerBlocks && existing != tiles_.end() && existing->second.stride != 0 &&
      existing->second.chunkMask != 0) {
    for(int cz = 0; cz < kTileChunks && !holdingForNative; ++cz) {
     for(int cx = 0; cx < kTileChunks; ++cx) {
      if((existing->second.chunkMask & (1u << (cz * kTileChunks + cx))) != 0 &&
         !sections.columnBuilt(tx * kTileChunks + cx, tz * kTileChunks + cz)) {
       holdingForNative = true;
       break;
      }
     }
    }
   }
   if(farthest < innerBlocks && !holdingForNative) continue;
   const double centerDx = minX + kTileBlocks * 0.5 - gridX;
   const double centerDz = minZ + kTileBlocks * 0.5 - gridZ;
   const double distance = std::sqrt(centerDx * centerDx + centerDz * centerDz);
   Tile& tile = tiles_[TileKey{tx, tz}];
   tile.seenFrame = frame_;
   const bool replacingWithNative = holdingForNative ||
                                    (nativeRadius_ < settings.nativeChunkRadius &&
                                     nearest < nativeBlocks - kNativeOverlapBlocks);
   const int stride = replacingWithNative && tile.stride != 0 ? tile.stride :
       tileStride(nearest, nativeBlocks, settings.lodQuality, memoryStride_);
   const int wantedStride = netherGeometry_ ? std::max(2, stride) : stride;
   if(tile.wantedStride != wantedStride) {
    tile.wantedStride = wantedStride;
    ++tile.generation;
   }
   wanted_.emplace_back(static_cast<int>(distance), TileKey{tx, tz});
  }
 }
 for(auto it = tiles_.begin(); it != tiles_.end();) {
  if(it->second.seenFrame != frame_) {
   if(it->second.chunkMask != 0) ++coverageRevision;
   releaseTile(it->second);
   it = tiles_.erase(it);
  } else {
   ++it;
  }
 }
 std::sort(wanted_.begin(), wanted_.end(), [](const auto& a, const auto& b) {
  if(a.first != b.first) return a.first < b.first;
  if(a.second.x != b.second.x) return a.second.x < b.second.x;
  return a.second.z < b.second.z;
 });
}
void DistantRenderer::updateClipDistance(const ChunkSectionSystem& sections) {
 const double innerBlocks = std::max(0, nativeRadius_ * 16 - kNativeOverlapBlocks);
 clipDistance_ = innerBlocks;
 for(const auto& [distance, key] : wanted_) {
  if(distance > innerBlocks + kTileBlocks * 1.4142135623730951 + 16.0) break;
  const Tile& tile = tiles_.at(key);
  if(tile.stride == 0 || tile.chunkMask == 0) continue;
  for(int cz = 0; cz < kTileChunks; ++cz) {
   for(int cx = 0; cx < kTileChunks; ++cx) {
    if((tile.chunkMask & (1u << (cz * kTileChunks + cx))) == 0) continue;
    const int chunkX = key.x * kTileChunks + cx;
    const int chunkZ = key.z * kTileChunks + cz;
    const double nearX = std::clamp(eyeX_, static_cast<double>(chunkX) * 16.0,
                                    static_cast<double>(chunkX + 1) * 16.0) - eyeX_;
    const double nearZ = std::clamp(eyeZ_, static_cast<double>(chunkZ) * 16.0,
                                    static_cast<double>(chunkZ + 1) * 16.0) - eyeZ_;
    const double nearest = std::sqrt(nearX * nearX + nearZ * nearZ);
    if(nearest < innerBlocks && !sections.columnBuilt(chunkX, chunkZ)) {
     clipDistance_ = std::min(clipDistance_, std::max(0.0, nearest - kNativeOverlapBlocks));
    }
   }
  }
 }
}
void DistantRenderer::updateTransitionCoverage(const ChunkSectionSystem& sections) {
 std::array<std::uint8_t, kCoverageSide * kCoverageSide * 2> coverage{};
 const int cameraX = MathHelper::floor(eyeX_) >> 4;
 const int cameraZ = MathHelper::floor(eyeZ_) >> 4;
 const int originX = cameraX - kCoverageSide / 2;
 const int originZ = cameraZ - kCoverageSide / 2;
 coverageOrigin_[0] = static_cast<float>(static_cast<double>(originX) * 16.0 - eyeX_);
 coverageOrigin_[1] = static_cast<float>(static_cast<double>(originZ) * 16.0 - eyeZ_);
 for(int z = 0; z < kCoverageSide; ++z) {
  for(int x = 0; x < kCoverageSide; ++x) {
   const int chunkX = originX + x;
   const int chunkZ = originZ + z;
   if(!coversColumn(chunkX, chunkZ)) continue;
   const auto offset = static_cast<std::size_t>((z * kCoverageSide + x) * 2);
   coverage[offset] = 255;
   if(!sections.empty() && world::columnInRange(chunkX, chunkZ, cameraX, cameraZ, nativeRadius_ + 1) &&
      sections.columnBuilt(chunkX, chunkZ)) coverage[offset + 1] = 255;
  }
 }
 if(coverage != transitionCoverage_) {
  transitionCoverage_ = coverage;
  coverageDirty_ = true;
 }
}
void DistantRenderer::update(World& world, client::texture::TextureManager& textures,
                             const ChunkSectionSystem& sections,
                             const option::RenderSettings& settings, double eyeX, double eyeY,
                             double eyeZ) {
 ++frame_;
 eyeX_ = eyeX;
 eyeY_ = eyeY;
 eyeZ_ = eyeZ;
 const bool netherGeometry = world.dimension != nullptr && world.dimension->isNether;
 if(!settings.distantTerrain) {
  if(!tiles_.empty()) reset();
  if(slabReady_) {
   slab_.destroy();
   slabReady_ = false;
  }
  for(auto& texture : depth_) texture.reset();
  coverageTexture_.reset();
  return;
 }
 if(world_ != &world || worldSeed_ != world.getSeed() || netherGeometry_ != netherGeometry) {
  if(!tiles_.empty() || biomes_ != nullptr || nativeRadius_ >= 0) reset();
  world_ = &world;
  worldSeed_ = world.getSeed();
  netherGeometry_ = netherGeometry;
 }
 const std::size_t slabBytes = static_cast<std::size_t>(settings.lodMemoryMiB) * 1024u * 1024u;
 if(slabReady_ && slab_.capacityBytes() != slabBytes) {
  const int previousRadius = nativeRadius_;
  reset();
  world_ = &world;
  worldSeed_ = world.getSeed();
  netherGeometry_ = netherGeometry;
  nativeRadius_ = previousRadius;
  slab_.destroy();
  slabReady_ = false;
 }
 if(!slabReady_) {
  slabReady_ = slab_.create(slabBytes);
  if(!slabReady_) return;
 }
 if(resourceGeneration_ != textures.resourceGeneration()) {
  const int atlasId = textures.getTextureId("/terrain.png");
  if(const client::texture::RasterImage* atlas = textures.getRasterImage(atlasId)) {
   auto updated = std::make_shared<BlockPalette>(defaultPalette());
   updatePaletteFromAtlas(*updated, *atlas);
   for(const int id : {8, 9, 10, 11}) {
    const auto* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(id)];
    if(block == nullptr) continue;
    auto& entry = updated->entries[static_cast<std::size_t>(id)];
    if(const auto color = textures.terrainTileColor(block->getTexture(1, 0)))
     std::copy(color->begin(), color->end(), entry.top);
    if(const auto color = textures.terrainTileColor(block->getTexture(2, 0)))
     std::copy(color->begin(), color->end(), entry.side);
   }
   palette_ = updated;
   resourceGeneration_ = textures.resourceGeneration();
   for(auto& [key, tile] : tiles_) tile.minimumGeneration = ++tile.generation;
  }
 }
 const float ambientOcclusion = settings.ambientOcclusionStrength;
 if(ambientOcclusion_ != ambientOcclusion) {
  ambientOcclusion_ = ambientOcclusion;
  for(auto& [key, tile] : tiles_) tile.minimumGeneration = ++tile.generation;
 }
 drainCompleted(4u * 1024u * 1024u);
 updateNativeRadius(sections, settings.nativeChunkRadius);
 ChunkSource* source = world.getChunkSource();
 if(source == nullptr) return;
 updateTileGrid(sections, settings);
 updateClipDistance(sections);
 updateTransitionCoverage(sections);
 auto& pool = util::concurrent::ThreadCoordinator::instance().pool(util::concurrent::Domain::Compute);
 pool_ = &pool;
 poolLifetime_ = pool.lifetimeToken();
 const int maxInFlight = static_cast<int>(std::clamp(pool.concurrencyLimit() / 2, 1u, 8u));
 if(inFlight_ >= maxInFlight) return;
 if(!netherGeometry && biomes_ == nullptr && world.getBiomeSource() != nullptr) {
  biomes_ = std::shared_ptr<BiomeSource>(world.getBiomeSource()->clone());
 }
 for(const auto& [distance, key] : wanted_) {
  Tile& tile = tiles_.at(key);
  if(tile.inFlight) continue;
  const int nearestX = std::clamp(grid_->cameraX, key.x * kTileChunks - 1,
                                  (key.x + 1) * kTileChunks);
  const int nearestZ = std::clamp(grid_->cameraZ, key.z * kTileChunks - 1,
                                  (key.z + 1) * kTileChunks);
  if(!world::columnInRange(nearestX, nearestZ, grid_->cameraX, grid_->cameraZ,
                            settings.residentChunkRadius + 2)) continue;
  const int baseX = key.x * kTileChunks - 1;
  const int baseZ = key.z * kTileChunks - 1;
  std::uint16_t readyChunkMask = 0;
  std::uint64_t readyPinMask = 0;
  for(int cz = 0; cz < kPinSide; ++cz) {
   for(int cx = 0; cx < kPinSide; ++cx) {
    const int chunkX = baseX + cx;
    const int chunkZ = baseZ + cz;
    if(!source->isChunkDataReady(chunkX, chunkZ) || !sections.columnLightingReady(chunkX, chunkZ) ||
       source->getChunkIfLoaded(chunkX, chunkZ) == nullptr) {
     continue;
    }
    readyPinMask |= 1ULL << (cz * kPinSide + cx);
    if(cx >= 1 && cx <= kTileChunks && cz >= 1 && cz <= kTileChunks) {
     readyChunkMask |= static_cast<std::uint16_t>(1u << ((cz - 1) * kTileChunks + cx - 1));
    }
   }
  }
  tile.readyPinMask = readyPinMask;
  if((readyChunkMask & ~tile.chunkMask) != 0 ||
     (readyPinMask & ~tile.builtPinMask) != 0) {
   ++tile.generation;
   tile.retryFrame = 0;
  }
 }
 collectBuildCandidates(static_cast<std::size_t>(maxInFlight - inFlight_));
 auto* cache = !world.isRemote() ? dynamic_cast<net::minecraft::world::chunk::ChunkCache*>(source) : nullptr;
 for(const auto& [priority, key] : buildCandidates_) {
  if(inFlight_ >= maxInFlight) break;
  Tile& tile = tiles_.at(key);
  auto pins = std::make_shared<PinnedChunks>();
  pins->baseX = key.x * kTileChunks - 1;
  pins->baseZ = key.z * kTileChunks - 1;
  bool any = false;
  std::uint64_t pinMask = 0;
  for(int cz = 0; cz < kPinSide; ++cz) {
   for(int cx = 0; cx < kPinSide; ++cx) {
    const int chunkX = pins->baseX + cx;
    const int chunkZ = pins->baseZ + cz;
    Chunk* chunk = source->isChunkDataReady(chunkX, chunkZ) && sections.columnLightingReady(chunkX, chunkZ)
                       ? source->getChunkIfLoaded(chunkX, chunkZ) : nullptr;
    if(chunk != nullptr && !chunk->tryAcquireRenderPin()) chunk = nullptr;
    pins->chunks[static_cast<std::size_t>(cz * kPinSide + cx)] = chunk;
    if(chunk != nullptr) pinMask |= 1ULL << (cz * kPinSide + cx);
    const bool own = cx >= 1 && cx <= kTileChunks && cz >= 1 && cz <= kTileChunks;
    if(own && chunk != nullptr) any = true;
   }
  }
  if(!any && cache == nullptr) {
   pins->release();
   tile.retryFrame = static_cast<int>(frame_) + kRetryFrames;
   continue;
  }
  tile.inFlight = true;
  tile.cancellation = std::make_shared<std::atomic<bool>>(false);
  const auto cancellation = tile.cancellation;
  tile.jobId = ++nextJobId_;
  const std::uint64_t jobId = tile.jobId;
  ++inFlight_;
  const std::shared_ptr<Shared> shared = shared_;
  const std::shared_ptr<const BiomeSource> biomes = biomes_;
  const std::shared_ptr<const BlockPalette> palette = palette_;
  const std::uint64_t generation = tile.generation;
  const int stride = tile.wantedStride;
  auto build = [shared, pins, biomes, palette, key, generation, stride, jobId, pinMask, netherGeometry, cancellation, ambientOcclusion]
      (const net::minecraft::world::chunk::ChunkCache::ChunkDataLoader& load) {
       Completed result;
       result.key = key;
       result.stride = stride;
       result.generation = generation;
       result.jobId = jobId;
       result.pinMask = pinMask;
       std::array<std::unique_ptr<Chunk>, kPinSide * kPinSide> detached;
       const ChunkLookup lookup = [&pins, &detached, &load, &shared, &cancellation](int chunkX, int chunkZ) -> const Chunk* {
        if(shared->cancelled.load(std::memory_order_acquire) || cancellation->load(std::memory_order_acquire)) return nullptr;
        const int cx = chunkX - pins->baseX;
        const int cz = chunkZ - pins->baseZ;
        if(cx < 0 || cz < 0 || cx >= kPinSide || cz >= kPinSide) return nullptr;
        const auto index = static_cast<std::size_t>(cz * kPinSide + cx);
        if(pins->chunks[index] != nullptr) return pins->chunks[index];
        if(load && detached[index] == nullptr) detached[index] = load(chunkX, chunkZ);
        return detached[index].get();
       };
       {
        const std::lock_guard lock(shared->mutex);
        if(!shared->spareMeshes.empty()) {
         result.mesh = std::move(shared->spareMeshes.back());
         shared->spareMeshes.pop_back();
        }
       }
       try {
        if(netherGeometry) {
         static thread_local NetherTileCells cells;
         if(extractNetherTile(lookup, key.x, key.z, stride, cells, true)) {
          meshNetherTile(cells, *palette, result.mesh);
          result.chunkMask = cells.chunkMask;
         }
        } else {
         static thread_local TileCells cells;
         if(extractTile(lookup, key.x, key.z, stride, cells, biomes.get(), true)) {
          meshTile(cells, *palette, result.mesh, ambientOcclusion);
          result.chunkMask = cells.chunkMask;
         }
        }
       } catch(...) {
        result.mesh = {};
        result.chunkMask = 0;
       }
       pins->release();
       const std::lock_guard lock(shared->mutex);
       shared->completed.push_back(std::move(result));
      };
  auto cancel = [shared, pins, key, generation, stride, jobId] {
       pins->release();
       Completed result;
       result.key = key;
       result.stride = stride;
       result.generation = generation;
       result.jobId = jobId;
       const std::lock_guard lock(shared->mutex);
       shared->completed.push_back(std::move(result));
      };
  if(cache != nullptr && pinMask != (1ULL << (kPinSide * kPinSide)) - 1ULL) {
   cache->readChunkDataAsync(std::move(build), priority + 4096, this, std::move(cancel));
  } else {
   pool.submit([build = std::move(build)] { build({}); }, priority, this, std::move(cancel));
  }
 }
}
void DistantRenderer::ensureDepthTextures(int width, int height) {
 if(depthWidth_ == width && depthHeight_ == height && depth_[0] && depth_[1]) return;
 for(gl::GlTexture& texture : depth_) {
  if(!texture) texture = gl::GlTexture(core::genTexture());
  core::bindTexture(static_cast<int>(texture.handle()));
  ::glTexImage2D(gl::cap::Texture2D, 0, 0x8CAC, width, height, 0, 0x1902, 0x1406, nullptr);
  ::glTexParameteri(gl::cap::Texture2D, 0x2801, 0x2600);
  ::glTexParameteri(gl::cap::Texture2D, 0x2800, 0x2600);
  ::glTexParameteri(gl::cap::Texture2D, 0x2802, 0x812F);
  ::glTexParameteri(gl::cap::Texture2D, 0x2803, 0x812F);
 }
 depthWidth_ = width;
 depthHeight_ = height;
}
void DistantRenderer::draw(Pipeline& pipeline, const option::RenderSettings& settings, const Frustum* culler,
                           bool water) {
 PackInstance* scene = pipeline.distantScenePack();
 if(!water && scene != nullptr) scene->lodCoverageTexture = 0;
 if(!settings.distantTerrain || !slabReady_ || tiles_.empty()) return;
 gl::ShaderProgram* program = pipeline.distantProgram(water);
 if(program == nullptr) return;
 if(!water && scene != nullptr && program->location("betaLodEnabled") >= 0) {
  const core::TextureBindScope textureScope;
  core::activeTexture(gl::tex::Texture0);
  if(!coverageTexture_) {
   coverageTexture_ = gl::GlTexture(core::genTexture());
   core::bindTexture(static_cast<int>(coverageTexture_.handle()));
   ::glTexImage2D(gl::cap::Texture2D, 0, 0x822B, kCoverageSide, kCoverageSide, 0, 0x8227, 0x1401,
                  transitionCoverage_.data());
   ::glTexParameteri(gl::cap::Texture2D, 0x2801, 0x2600);
   ::glTexParameteri(gl::cap::Texture2D, 0x2800, 0x2600);
   ::glTexParameteri(gl::cap::Texture2D, 0x2802, 0x812F);
   ::glTexParameteri(gl::cap::Texture2D, 0x2803, 0x812F);
  } else if(coverageDirty_) {
   core::bindTexture(static_cast<int>(coverageTexture_.handle()));
   ::glTexSubImage2D(gl::cap::Texture2D, 0, 0, 0, kCoverageSide, kCoverageSide, 0x8227, 0x1401,
                     transitionCoverage_.data());
  }
  coverageDirty_ = false;
  scene->lodCoverageTexture = coverageTexture_.handle();
  std::copy(std::begin(coverageOrigin_), std::end(coverageOrigin_), scene->lodCoverageOrigin);
  scene->lodTransition[0] = static_cast<float>(std::max(0, nativeRadius_ * 16 - kNativeOverlapBlocks));
  scene->lodTransition[1] = static_cast<float>(std::max(1, nativeRadius_ * 16 - 16));
 }
 const FrameRenderCamera& frame = core::cameraFrame();
 util::math::Matrix4f projection;
 if(scene != nullptr) {
  ensureDepthTextures(scene->colorTargets.width(), scene->colorTargets.height());
  scene->dhDepthTexture = depth_[0].handle();
  scene->dhDepthTextureNoTranslucent = depth_[1].handle();
  scene->colorTargets.bindGbuffersWithDepth(depth_[0].handle());
  if(!water) {
   const core::DepthScope clearDepth(true, true);
   ::glClear(0x00000100);
  }
  FrameRenderCamera dhCamera = frame;
  dhCamera.nearPlane = frame.dhNearPlane;
  dhCamera.farPlane = frame.dhFarPlane;
  buildCameraProjection(projection.data(), dhCamera);
 } else {
  projection = core::drawProjection();
 }
 pipeline.bindDistantProgram(*program, water);
 const util::math::Matrix4f& modelView = core::drawModelView();
 util::math::Matrix4f modelViewInverse = modelView;
 modelViewInverse.invert();
 util::math::Matrix4f projectionInverse = projection;
 projectionInverse.invert();
 float normal[9]{};
 normalMatrix(modelView, normal);
 program->setMatrix4("iris_ModelViewMatrix", modelView.m);
 program->setMatrix4("iris_ModelViewMatrixInverse", modelViewInverse.m);
 program->setMatrix4("iris_ProjectionMatrix", projection.m);
 program->setMatrix4("iris_ProjectionMatrixInverse", projectionInverse.m);
 program->setMatrix3At(program->location("iris_NormalMatrix"), normal);
 const core::FogUniforms& fog = core::fog();
 program->set1f("betaFogStart", fog.start);
 program->set1f("betaFogEnd", fog.end);
 program->set1i("customFog", fog.custom ? 1 : 0);
 program->set1i("fogShape", fog.enabled ? fog.shape : -1);
 program->set1f("mircoOffset", 0.01f);
 program->set1f("clipDistance", static_cast<float>(clipDistance_));
 const int modelOffset = program->location("modelOffset");
 const core::DepthScope depth(true, true);
 const core::CullScope cull(!water);
 const core::BlendScope blend(water);
 if(scene == nullptr) {
  core::polygonOffset(1.0f, 4.0f);
  core::enablePolygonOffset();
 }
 gl::GLCore::bindVertexArray(slab_.vao());
 int drawn = 0;
 for(std::size_t index = 0; index < wanted_.size(); ++index) {
  const TileKey& key = wanted_[water ? wanted_.size() - index - 1 : index].second;
  const auto found = tiles_.find(key);
  if(found == tiles_.end()) continue;
  const Tile& tile = found->second;
  const std::array allocations = water ? std::array{settings.renderWater ? &tile.water : nullptr, &tile.ice}
                                        : std::array{&tile.opaque, static_cast<const std::optional<DistantSlab::Allocation>*>(nullptr)};
  if(std::none_of(allocations.begin(), allocations.end(), [](const auto* allocation) {
      return allocation != nullptr && allocation->has_value() && (*allocation)->vertexCount != 0;
     })) continue;
  const double minX = static_cast<double>(key.x * kTileBlocks);
  const double minZ = static_cast<double>(key.z * kTileBlocks);
  if(culler != nullptr &&
     !culler->isVisible(minX, static_cast<double>(tile.minY), minZ, minX + kTileBlocks,
                        static_cast<double>(tile.maxY) + 1.0, minZ + kTileBlocks)) {
   continue;
  }
  const float offset[3] = {static_cast<float>(minX - eyeX_), static_cast<float>(-eyeY_),
                           static_cast<float>(minZ - eyeZ_)};
  program->set3fAt(modelOffset, offset);
  for(const auto* allocation : allocations) {
   if(allocation == nullptr || !allocation->has_value() || (*allocation)->vertexCount == 0) continue;
   const core::CullScope surfaceCull(!water || allocation == &tile.ice);
   quad_index::ensure((*allocation)->vertexCount);
   gl::GLCore::drawElementsBaseVertex(gl::prim::Triangles, static_cast<int>((*allocation)->vertexCount / 4u * 6u),
                                      gl::pixel::UnsignedInt, nullptr, static_cast<int>((*allocation)->baseVertex()));
   ++drawn;
  }
 }
 core::unbindVertexArray();
 if(scene == nullptr) {
  core::disablePolygonOffset();
 }
 if(scene != nullptr) {
  if(!water) {
   core::activeTexture(gl::tex::Texture0);
   core::bindTexture(static_cast<int>(depth_[1].handle()));
   ::glCopyTexSubImage2D(gl::cap::Texture2D, 0, 0, 0, 0, 0, depthWidth_, depthHeight_);
  }
  scene->colorTargets.bindGbuffersWithDepth(0);
 }
 if(!water) drawnLastFrame_ = drawn;
}
void DistantRenderer::renderOpaque(Pipeline& pipeline, const option::RenderSettings& settings, const Frustum* culler) {
 draw(pipeline, settings, culler, false);
}
void DistantRenderer::renderWater(Pipeline& pipeline, const option::RenderSettings& settings, const Frustum* culler) {
 draw(pipeline, settings, culler, true);
}
std::string DistantRenderer::debugInfo() const {
 constexpr std::size_t kMiB = 1024u * 1024u;
 return "Distant: tiles " + std::to_string(tiles_.size()) + ", drawn " + std::to_string(drawnLastFrame_) +
        ", flight " + std::to_string(inFlight_) + ", slab " + std::to_string(slab_.usedBytes() / kMiB) + "/" +
        std::to_string(slab_.capacityBytes() / kMiB) + " MiB";
}
bool DistantRenderer::coversColumn(int chunkX, int chunkZ) const noexcept {
 const int tileX = MathHelper::floorDiv(chunkX, kTileChunks);
 const int tileZ = MathHelper::floorDiv(chunkZ, kTileChunks);
 const auto it = tiles_.find(TileKey{tileX, tileZ});
 if(it == tiles_.end() || it->second.stride == 0) return false;
 const int localX = chunkX - tileX * kTileChunks;
 const int localZ = chunkZ - tileZ * kTileChunks;
 return (it->second.chunkMask & (1u << (localZ * kTileChunks + localX))) != 0;
}
} // namespace net::minecraft::client::render::distant
