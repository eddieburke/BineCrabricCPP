#include "net/minecraft/client/render/chunk/TerrainRegion.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include "net/minecraft/client/debug/VTuneTrace.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
namespace net::minecraft::client::render::chunk {
namespace {
constexpr unsigned kArrayBuffer = 0x8892;
constexpr unsigned kCopyReadBuffer = 0x8F36;
constexpr unsigned kCopyWriteBuffer = 0x8F37;
constexpr unsigned kDynamicDraw = 0x88E8;
constexpr unsigned kStreamDraw = 0x88E0;
constexpr std::size_t kInitialVertices = 4096;
constexpr std::size_t kAllocationQuantum = 256;
} // namespace
TerrainRegion::TerrainRegion(int originX, int originY, int originZ)
    : originX_(originX), originY_(originY), originZ_(originZ) {
 sections_.reserve(256);
}
TerrainRegion::~TerrainRegion() {
 for(LayerArena& arena : layers_) {
  if(arena.vao != 0 && gl::GLCore::deleteVertexArrays != nullptr) {
   gl::GLCore::deleteVertexArrays(1, &arena.vao);
  }
  if(arena.handle != 0 && gl::GLCore::deleteBuffers != nullptr) {
   gl::GLCore::deleteBuffers(1, &arena.handle);
  }
  if(arena.staging != 0 && gl::GLCore::deleteBuffers != nullptr) {
   gl::GLCore::deleteBuffers(1, &arena.staging);
  }
 }
}
void TerrainRegion::addSection(ChunkBuilder* section) {
 if(section != nullptr) {
  sections_.push_back(section);
 }
}
void TerrainRegion::removeSection(ChunkBuilder* section) {
 sections_.erase(std::remove(sections_.begin(), sections_.end(), section), sections_.end());
}
bool TerrainRegion::ensureCapacity(LayerArena& arena, std::size_t requiredVertices) {
 if(requiredVertices <= arena.capacityVertices && arena.handle != 0 && arena.vao != 0) {
  return true;
 }
 if(gl::GLCore::genBuffers == nullptr || gl::GLCore::bindBuffer == nullptr ||
    gl::GLCore::bufferData == nullptr || gl::GLCore::deleteBuffers == nullptr ||
    gl::GLCore::genVertexArrays == nullptr || gl::GLCore::deleteVertexArrays == nullptr ||
    (arena.handle != 0 && gl::GLCore::copyBufferSubData == nullptr)) {
  return false;
 }
 std::size_t newCapacity = std::max(kInitialVertices, arena.capacityVertices);
 while(newCapacity < requiredVertices) {
  newCapacity *= 2;
 }
 if(!quad_index::ensure(kInitialVertices)) {
  return false;
 }
 VT_TRACE_COUNTER("Copies", 1);
 // Separated from Copies so the HUD says how often arenas regrow per frame and
 // how much they copy, rather than folding both into one shared counter.
 VT_TRACE_COUNTER("ArenaGrows", 1);
 VT_TRACE_COUNTER("ArenaGrowVertices", static_cast<std::uint64_t>(arena.tailVertex));
 unsigned newHandle = 0;
 unsigned newVao = 0;
 gl::GLCore::genBuffers(1, &newHandle);
 gl::GLCore::bindBuffer(kArrayBuffer, newHandle);
 gl::GLCore::bufferData(kArrayBuffer,
                        static_cast<intptr_t>(newCapacity * sizeof(TessellatorVertex)),
                        nullptr,
                        kDynamicDraw);
 gl::GLCore::genVertexArrays(1, &newVao);
 if(newHandle == 0 || newVao == 0 ||
    !core::configureIndexedVao(newVao,
                               newHandle,
                               quad_index::handle(),
                               0,
                               static_cast<int>(sizeof(TessellatorVertex)),
                               true,
                               true)) {
  if(newVao != 0) gl::GLCore::deleteVertexArrays(1, &newVao);
  if(newHandle != 0) gl::GLCore::deleteBuffers(1, &newHandle);
  return false;
 }
 if(arena.handle != 0 && arena.tailVertex != 0) {
  gl::GLCore::bindBuffer(kCopyReadBuffer, arena.handle);
  gl::GLCore::bindBuffer(kCopyWriteBuffer, newHandle);
  gl::GLCore::copyBufferSubData(kCopyReadBuffer,
                                kCopyWriteBuffer,
                                0,
                                0,
                                static_cast<intptr_t>(arena.tailVertex * sizeof(TessellatorVertex)));
  gl::GLCore::bindBuffer(kCopyReadBuffer, 0);
  gl::GLCore::bindBuffer(kCopyWriteBuffer, 0);
 }
 if(arena.vao != 0) gl::GLCore::deleteVertexArrays(1, &arena.vao);
 if(arena.handle != 0) gl::GLCore::deleteBuffers(1, &arena.handle);
 arena.handle = newHandle;
 arena.vao = newVao;
 arena.capacityVertices = newCapacity;
 return true;
}
bool TerrainRegion::acquire(LayerArena& arena,
                            std::size_t requiredVertices,
                            TerrainAllocation& allocation) {
 const std::size_t capacity = std::bit_ceil(std::max(requiredVertices, kAllocationQuantum));
 const int sizeClass = std::countr_zero(capacity);
 if(sizeClass < kMinSizeClass || sizeClass > kMaxSizeClass) {
  return false;
 }
 std::unordered_set<std::size_t>& exact = freeList(arena, sizeClass);
 if(const auto it = exact.begin(); it != exact.end()) {
  allocation.firstVertex = *it;
  allocation.capacityVertices = capacity;
  exact.erase(it);
  return true;
 }
 // Split the smallest larger block; each half is aligned because its parent was,
 // which is what keeps the buddy identity in releaseRange valid.
 for(int larger = sizeClass + 1; larger <= kMaxSizeClass; ++larger) {
  std::unordered_set<std::size_t>& donor = freeList(arena, larger);
  const auto it = donor.begin();
  if(it == donor.end()) {
   continue;
  }
  const std::size_t first = *it;
  donor.erase(it);
  for(int split = larger; split > sizeClass; --split) {
   freeList(arena, split - 1).insert(first + (std::size_t{1} << (split - 1)));
  }
  allocation.firstVertex = first;
  allocation.capacityVertices = capacity;
  return true;
 }
 // Cutting the tail has to land on a multiple of `capacity` or the buddy address
 // stops being `first ^ capacity`. The skipped gap is donated back as aligned
 // power-of-two blocks rather than stranded.
 const std::size_t mask = capacity - 1;
 if(arena.tailVertex > std::numeric_limits<std::size_t>::max() - mask) {
  return false;
 }
 const std::size_t aligned = (arena.tailVertex + mask) & ~mask;
 if(aligned > std::numeric_limits<std::size_t>::max() - capacity ||
    !ensureCapacity(arena, aligned + capacity)) {
  return false;
 }
 for(std::size_t gap = arena.tailVertex; gap < aligned;) {
  const std::size_t block = std::bit_floor(std::min(gap & (~gap + 1), aligned - gap));
  const int blockClass = std::countr_zero(block);
  if(blockClass < kMinSizeClass || blockClass > kMaxSizeClass) {
   break;
  }
  freeList(arena, blockClass).insert(gap);
  gap += block;
 }
 allocation.firstVertex = aligned;
 allocation.capacityVertices = capacity;
 arena.tailVertex = aligned + capacity;
 return true;
}
void TerrainRegion::releaseRange(LayerArena& arena, TerrainAllocation& allocation) noexcept {
 if(allocation.capacityVertices == 0) {
  allocation = {};
  return;
 }
 std::size_t first = allocation.firstVertex;
 int sizeClass = std::countr_zero(allocation.capacityVertices);
 allocation = {};
 if(sizeClass < kMinSizeClass || sizeClass > kMaxSizeClass) {
  return;
 }
 while(sizeClass < kMaxSizeClass) {
  const std::size_t buddy = first ^ (std::size_t{1} << sizeClass);
  if(freeList(arena, sizeClass).erase(buddy) == 0) {
   break;
  }
  first = std::min(first, buddy);
  ++sizeClass;
 }
 freeList(arena, sizeClass).insert(first);
 trimTail(arena);
}
// Merging leaves at most one free block touching the tail per pass, so hand the
// space back to the allocator's high-water mark instead of letting ensureCapacity
// grow past it on the next large request.
void TerrainRegion::trimTail(LayerArena& arena) noexcept {
 for(bool shrank = true; shrank;) {
  shrank = false;
  for(int sizeClass = kMaxSizeClass; sizeClass >= kMinSizeClass; --sizeClass) {
   const std::size_t span = std::size_t{1} << sizeClass;
   if(arena.tailVertex < span || (arena.tailVertex & (span - 1)) != 0) {
    continue;
   }
   if(freeList(arena, sizeClass).erase(arena.tailVertex - span) == 0) {
    continue;
   }
   arena.tailVertex -= span;
   shrank = true;
   break;
  }
 }
}
bool TerrainRegion::upload(int layer,
                           TerrainAllocation& allocation,
                           std::span<const TessellatorVertex> vertices) {
 if(layer < 0 || layer >= terrain_layer::Count || vertices.empty() ||
    gl::GLCore::bufferSubData == nullptr || gl::GLCore::bufferData == nullptr ||
    gl::GLCore::genBuffers == nullptr || gl::GLCore::copyBufferSubData == nullptr) {
  return false;
 }
 LayerArena& arena = layers_[static_cast<std::size_t>(layer)];
 const std::size_t wantedCapacity = std::bit_ceil(std::max(vertices.size(), kAllocationQuantum));
 if(allocation.capacityVertices < vertices.size() || allocation.capacityVertices >= wantedCapacity * 4) {
  releaseRange(arena, allocation);
  if(!acquire(arena, vertices.size(), allocation)) {
   return false;
  }
 }
 if(arena.handle == 0 && !ensureCapacity(arena, allocation.firstVertex + allocation.capacityVertices)) {
  return false;
 }
 if(arena.staging == 0) {
  gl::GLCore::genBuffers(1, &arena.staging);
  if(arena.staging == 0) {
   return false;
  }
 }
 gl::GLCore::bindBuffer(kCopyReadBuffer, arena.staging);
 gl::GLCore::bufferData(kCopyReadBuffer,
                        static_cast<intptr_t>(std::bit_ceil(vertices.size()) * sizeof(TessellatorVertex)),
                        nullptr,
                        kStreamDraw);
 gl::GLCore::bufferSubData(kCopyReadBuffer, 0, static_cast<intptr_t>(vertices.size_bytes()), vertices.data());
 gl::GLCore::bindBuffer(kCopyWriteBuffer, arena.handle);
 gl::GLCore::copyBufferSubData(kCopyReadBuffer,
                               kCopyWriteBuffer,
                               0,
                               static_cast<intptr_t>(allocation.firstVertex * sizeof(TessellatorVertex)),
                               static_cast<intptr_t>(vertices.size_bytes()));
 gl::GLCore::bindBuffer(kCopyReadBuffer, 0);
 gl::GLCore::bindBuffer(kCopyWriteBuffer, 0);
 allocation.vertexCount = static_cast<int>(vertices.size());
 return true;
}
void TerrainRegion::release(int layer, TerrainAllocation& allocation) noexcept {
 if(layer < 0 || layer >= terrain_layer::Count) {
  allocation = {};
  return;
 }
 releaseRange(layers_[static_cast<std::size_t>(layer)], allocation);
}
int TerrainRegion::drawLayer(int layer, std::span<const TerrainAllocation* const> allocations) {
 if(layer < 0 || layer >= terrain_layer::Count || allocations.empty()) {
  return 0;
 }
 LayerArena& arena = layers_[static_cast<std::size_t>(layer)];
 if(arena.handle == 0 || arena.vao == 0) {
  return 0;
 }
 std::size_t totalVertices = 0;
 std::size_t maxVertices = 0;
 arena.indexCounts.clear();
 arena.baseVertices.clear();
 arena.indexCounts.reserve(allocations.size());
 arena.baseVertices.reserve(allocations.size());
 for(const TerrainAllocation* allocation : allocations) {
  if(allocation == nullptr || !allocation->valid() || allocation->vertexCount < 4) {
   continue;
  }
  const int quadVertices = allocation->vertexCount - allocation->vertexCount % 4;
  arena.indexCounts.push_back((quadVertices / 4) * 6);
  arena.baseVertices.push_back(static_cast<int>(allocation->firstVertex));
  maxVertices = std::max(maxVertices, static_cast<std::size_t>(quadVertices));
  totalVertices += static_cast<std::size_t>(quadVertices);
 }
 if(arena.indexCounts.empty() || !quad_index::ensure(maxVertices)) {
  return 0;
 }
 core::RenderPass pass;
 pass.modelView = core::drawModelView();
 pass.projection = core::drawProjection();
 core::applyPendingTerrain(pass);
 pass.buffer = arena.handle;
 pass.vertexCount = totalVertices;
 pass.stride = static_cast<int>(sizeof(TessellatorVertex));
 pass.hasTexture = true;
 pass.hasNormals = true;
 return core::submitIndexedQuadsBatch(pass, arena.vao, arena.indexCounts, arena.baseVertices);
}
} // namespace net::minecraft::client::render::chunk
