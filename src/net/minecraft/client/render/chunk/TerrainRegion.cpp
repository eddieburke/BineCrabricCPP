#include "net/minecraft/client/render/chunk/TerrainRegion.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
namespace net::minecraft::client::render::chunk {
namespace {
constexpr unsigned kArrayBuffer = 0x8892;
constexpr unsigned kCopyReadBuffer = 0x8F36;
constexpr unsigned kCopyWriteBuffer = 0x8F37;
constexpr unsigned kDynamicDraw = 0x88E8;
constexpr std::size_t kInitialVertices = 4096;
constexpr std::size_t kAllocationQuantum = 256;
}
TerrainRegion::TerrainRegion(int originX, int originY, int originZ)
    : originX_(originX), originY_(originY), originZ_(originZ) {
 sections_.reserve(256);
 for(LayerArena& arena : layers_) {
  arena.freeRanges.reserve(256);
 }
}
TerrainRegion::~TerrainRegion() {
 for(LayerArena& arena : layers_) {
  if(arena.vao != 0 && gl::GLCore::deleteVertexArrays != nullptr) {
   gl::GLCore::deleteVertexArrays(1, &arena.vao);
  }
  if(arena.handle != 0 && gl::GLCore::deleteBuffers != nullptr) {
   gl::GLCore::deleteBuffers(1, &arena.handle);
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
 auto best = arena.freeRanges.end();
 for(auto it = arena.freeRanges.begin(); it != arena.freeRanges.end(); ++it) {
  if(it->capacity >= capacity &&
     (best == arena.freeRanges.end() || it->capacity < best->capacity)) {
   best = it;
  }
 }
 if(best != arena.freeRanges.end()) {
  allocation.firstVertex = best->first;
  allocation.capacityVertices = capacity;
  if(best->capacity == capacity) {
   arena.freeRanges.erase(best);
  } else {
   best->first += capacity;
   best->capacity -= capacity;
  }
  return true;
 }
 if(arena.tailVertex > std::numeric_limits<std::size_t>::max() - capacity ||
    !ensureCapacity(arena, arena.tailVertex + capacity)) {
  return false;
 }
 allocation.firstVertex = arena.tailVertex;
 allocation.capacityVertices = capacity;
 arena.tailVertex += capacity;
 return true;
}
void TerrainRegion::releaseRange(LayerArena& arena, TerrainAllocation& allocation) noexcept {
 if(allocation.capacityVertices == 0) {
  allocation = {};
  return;
 }
 arena.freeRanges.push_back(Range{allocation.firstVertex, allocation.capacityVertices});
 std::sort(arena.freeRanges.begin(), arena.freeRanges.end(), [](const Range& a, const Range& b) {
  return a.first < b.first;
 });
 std::size_t write = 0;
 for(const Range& range : arena.freeRanges) {
  if(write != 0 && arena.freeRanges[write - 1].first + arena.freeRanges[write - 1].capacity == range.first) {
   arena.freeRanges[write - 1].capacity += range.capacity;
  } else {
   arena.freeRanges[write++] = range;
  }
 }
 arena.freeRanges.resize(write);
 while(!arena.freeRanges.empty()) {
  const Range& last = arena.freeRanges.back();
  if(last.first + last.capacity != arena.tailVertex) {
   break;
  }
  arena.tailVertex = last.first;
  arena.freeRanges.pop_back();
 }
 allocation = {};
}
bool TerrainRegion::upload(int layer,
                           TerrainAllocation& allocation,
                           std::span<const TessellatorVertex> vertices) {
 if(layer < 0 || layer >= terrain_layer::Count || vertices.empty() ||
    gl::GLCore::bufferSubData == nullptr) {
  return false;
 }
 LayerArena& arena = layers_[static_cast<std::size_t>(layer)];
 if(allocation.capacityVertices < vertices.size()) {
  releaseRange(arena, allocation);
  if(!acquire(arena, vertices.size(), allocation)) {
   return false;
  }
 } else if(arena.handle == 0 && !ensureCapacity(arena, allocation.firstVertex + allocation.capacityVertices)) {
  return false;
 }
 gl::GLCore::bindBuffer(kArrayBuffer, arena.handle);
 gl::GLCore::bufferSubData(kArrayBuffer,
                           static_cast<intptr_t>(allocation.firstVertex * sizeof(TessellatorVertex)),
                           static_cast<intptr_t>(vertices.size_bytes()),
                           vertices.data());
 gl::GLCore::bindBuffer(kArrayBuffer, 0);
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
 arena.indexCounts.clear();
 arena.baseVertices.clear();
 std::size_t maxVertices = 0;
 std::size_t totalVertices = 0;
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
}
