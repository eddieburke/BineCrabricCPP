#include "net/minecraft/client/render/chunk/TerrainRegion.hpp"
#include <algorithm>
#include <cstring>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
namespace net::minecraft::client::render::chunk {
namespace {
constexpr unsigned kArrayBuffer = 0x8892;
constexpr unsigned kCopyReadBuffer = 0x8F36;
constexpr unsigned kCopyWriteBuffer = 0x8F37;
constexpr unsigned kDynamicDraw = 0x88E8;
constexpr unsigned kDynamicStorageBit = 0x0100;
constexpr std::size_t kMinCapacityVertices = 1024;
constexpr std::size_t kVertexBytes = sizeof(TerrainVertex);
[[nodiscard]] std::size_t roundToGranule(std::size_t vertices) noexcept {
 return (vertices + TerrainRegion::kGranuleVertices - 1) / TerrainRegion::kGranuleVertices *
        TerrainRegion::kGranuleVertices;
}
[[nodiscard]] std::uint16_t packUnorm16(float value) noexcept {
 return static_cast<std::uint16_t>(std::clamp(value, 0.0f, 1.0f) * 65535.0f + 0.5f);
}
[[nodiscard]] std::int8_t packSnorm8(std::int16_t value) noexcept {
 const int scaled = static_cast<int>(value) * 127;
 return static_cast<std::int8_t>((scaled + (scaled < 0 ? -16383 : 16383)) / 32767);
}
} // namespace
std::span<const std::byte> packTerrainVertices(std::span<TessellatorVertex> vertices) noexcept {
 std::byte* const base = reinterpret_cast<std::byte*>(vertices.data());
 for(std::size_t i = 0; i < vertices.size(); ++i) {
  const TessellatorVertex source = vertices[i];
  TerrainVertex packed;
  packed.x = source.x;
  packed.y = source.y;
  packed.z = source.z;
  packed.u = source.u;
  packed.v = source.v;
  packed.color = source.color;
  packed.normal = source.normal;
  packed.midBlock = source.midBlock;
  packed.light = source.light;
  packed.entity[0] = source.entity[0];
  packed.entity[1] = source.entity[1];
  packed.midTexCoord[0] = packUnorm16(source.midU);
  packed.midTexCoord[1] = packUnorm16(source.midV);
  for(int c = 0; c < 4; ++c) packed.tangent[c] = packSnorm8(source.tangent[c]);
  std::memcpy(base + i * kVertexBytes, &packed, kVertexBytes);
 }
 return {base, vertices.size() * kVertexBytes};
}
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
bool TerrainRegion::rebuild(int layer, std::size_t capacityVertices) {
 LayerArena& arena = layers_[static_cast<std::size_t>(layer)];
 if(capacityVertices == 0) {
  if(arena.vao != 0) gl::GLCore::deleteVertexArrays(1, &arena.vao);
  if(arena.handle != 0) gl::GLCore::deleteBuffers(1, &arena.handle);
  arena.vao = 0;
  arena.handle = 0;
  arena.capacityVertices = 0;
  arena.freeRanges.clear();
  return true;
 }
 if(gl::GLCore::genBuffers == nullptr || gl::GLCore::bindBuffer == nullptr ||
    gl::GLCore::deleteBuffers == nullptr || gl::GLCore::genVertexArrays == nullptr ||
    gl::GLCore::deleteVertexArrays == nullptr || gl::GLCore::copyBufferSubData == nullptr ||
    (gl::GLCore::bufferStorage == nullptr && gl::GLCore::bufferData == nullptr) ||
    !quad_index::ensure(kGranuleVertices)) {
  return false;
 }
 unsigned newHandle = 0;
 unsigned newVao = 0;
 gl::GLCore::genBuffers(1, &newHandle);
 gl::GLCore::bindBuffer(kArrayBuffer, newHandle);
 const auto bytes = static_cast<intptr_t>(capacityVertices * kVertexBytes);
 if(gl::GLCore::bufferStorage != nullptr) {
  gl::GLCore::bufferStorage(kArrayBuffer, bytes, nullptr, kDynamicStorageBit);
 } else {
  gl::GLCore::bufferData(kArrayBuffer, bytes, nullptr, kDynamicDraw);
 }
 gl::GLCore::genVertexArrays(1, &newVao);
 if(newHandle == 0 || newVao == 0 ||
    !core::configureIndexedVao(newVao, newHandle, quad_index::handle(), static_cast<int>(kVertexBytes),
                               vertex_abi::TerrainFormats)) {
  if(newVao != 0) gl::GLCore::deleteVertexArrays(1, &newVao);
  if(newHandle != 0) gl::GLCore::deleteBuffers(1, &newHandle);
  return false;
 }
 std::size_t cursor = 0;
 if(arena.handle != 0) {
  gl::GLCore::bindBuffer(kCopyReadBuffer, arena.handle);
  gl::GLCore::bindBuffer(kCopyWriteBuffer, newHandle);
  for(ChunkBuilder* section : sections_) {
   TerrainAllocation& allocation = section->terrainAllocations_[static_cast<std::size_t>(layer)];
   if(allocation.capacityVertices == 0) {
    continue;
   }
   if(allocation.vertexCount > 0) {
    gl::GLCore::copyBufferSubData(kCopyReadBuffer,
                                  kCopyWriteBuffer,
                                  static_cast<intptr_t>(allocation.firstVertex * kVertexBytes),
                                  static_cast<intptr_t>(cursor * kVertexBytes),
                                  static_cast<intptr_t>(static_cast<std::size_t>(allocation.vertexCount) *
                                                        kVertexBytes));
   }
   allocation.firstVertex = cursor;
   cursor += allocation.capacityVertices;
  }
  gl::GLCore::deleteVertexArrays(1, &arena.vao);
  gl::GLCore::deleteBuffers(1, &arena.handle);
 }
 arena.handle = newHandle;
 arena.vao = newVao;
 arena.capacityVertices = capacityVertices;
 arena.liveVertices = cursor;
 arena.freeRanges.clear();
 if(cursor < capacityVertices) {
  arena.freeRanges.emplace(cursor, capacityVertices - cursor);
 }
 return true;
}
bool TerrainRegion::acquire(int layer, std::size_t vertices, TerrainAllocation& allocation) {
 LayerArena& arena = layers_[static_cast<std::size_t>(layer)];
 const std::size_t needed = roundToGranule(vertices);
 auto best = arena.freeRanges.end();
 for(auto it = arena.freeRanges.begin(); it != arena.freeRanges.end(); ++it) {
  if(it->second >= needed && (best == arena.freeRanges.end() || it->second < best->second)) {
   best = it;
  }
 }
 if(best == arena.freeRanges.end()) {
  const std::size_t required = arena.liveVertices + needed;
  if(!rebuild(layer, std::max(kMinCapacityVertices, roundToGranule(required + required / 2)))) {
   return false;
  }
  best = std::prev(arena.freeRanges.end());
 }
 const std::size_t first = best->first;
 const std::size_t length = best->second;
 arena.freeRanges.erase(best);
 if(length > needed) {
  arena.freeRanges.emplace(first + needed, length - needed);
 }
 arena.liveVertices += needed;
 allocation.firstVertex = first;
 allocation.capacityVertices = needed;
 allocation.vertexCount = 0;
 return true;
}
void TerrainRegion::releaseRange(LayerArena& arena, TerrainAllocation& allocation) noexcept {
 if(allocation.capacityVertices == 0) {
  allocation = {};
  return;
 }
 std::size_t first = allocation.firstVertex;
 std::size_t length = allocation.capacityVertices;
 arena.liveVertices -= length;
 allocation = {};
 auto next = arena.freeRanges.lower_bound(first);
 if(next != arena.freeRanges.begin()) {
  const auto previous = std::prev(next);
  if(previous->first + previous->second == first) {
   first = previous->first;
   length += previous->second;
   arena.freeRanges.erase(previous);
  }
 }
 if(next != arena.freeRanges.end() && first + length == next->first) {
  length += next->second;
  arena.freeRanges.erase(next);
 }
 arena.freeRanges.emplace(first, length);
}
void TerrainRegion::shrinkIfSparse(int layer) {
 const LayerArena& arena = layers_[static_cast<std::size_t>(layer)];
 if(arena.liveVertices == 0) {
  rebuild(layer, 0);
  return;
 }
 if(arena.capacityVertices > kMinCapacityVertices && arena.liveVertices * 3 < arena.capacityVertices) {
  rebuild(layer, std::max(kMinCapacityVertices, roundToGranule(arena.liveVertices + arena.liveVertices / 2)));
 }
}
bool TerrainRegion::upload(int layer, TerrainAllocation& allocation, std::span<TessellatorVertex> vertices) {
 if(layer < 0 || layer >= terrain_layer::Count || vertices.empty() || gl::GLCore::bufferSubData == nullptr) {
  return false;
 }
 LayerArena& arena = layers_[static_cast<std::size_t>(layer)];
 const std::size_t needed = roundToGranule(vertices.size());
 if(allocation.capacityVertices < needed || allocation.capacityVertices > needed * 2) {
  TerrainAllocation replacement;
  if(!acquire(layer, vertices.size(), replacement)) {
   return false;
  }
  releaseRange(arena, allocation);
  allocation = replacement;
 }
 const std::span<const std::byte> packed = packTerrainVertices(vertices);
 gl::GLCore::bindBuffer(kArrayBuffer, arena.handle);
 gl::GLCore::bufferSubData(kArrayBuffer,
                           static_cast<intptr_t>(allocation.firstVertex * kVertexBytes),
                           static_cast<intptr_t>(packed.size()),
                           packed.data());
 allocation.vertexCount = static_cast<int>(vertices.size());
 return true;
}
void TerrainRegion::release(int layer, TerrainAllocation& allocation) noexcept {
 if(layer < 0 || layer >= terrain_layer::Count) {
  allocation = {};
  return;
 }
 const bool held = allocation.capacityVertices != 0;
 releaseRange(layers_[static_cast<std::size_t>(layer)], allocation);
 if(held) {
  shrinkIfSparse(layer);
 }
}
std::size_t TerrainRegion::gpuBytes() const noexcept {
 std::size_t vertices = 0;
 for(const LayerArena& arena : layers_) vertices += arena.capacityVertices;
 return vertices * kVertexBytes;
}
std::size_t TerrainRegion::liveBytes() const noexcept {
 std::size_t vertices = 0;
 for(const LayerArena& arena : layers_) vertices += arena.liveVertices;
 return vertices * kVertexBytes;
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
 // see src/net/minecraft/client/render/Tessellator.cpp Tessellator::vertex (worker meshes: overlay 0, iris_Entity -1)
 if(gl::GLCore::vertexAttribI4i != nullptr) {
  gl::GLCore::vertexAttribI4i(vertex_abi::Overlay, 0, 0, 0, 1);
  gl::GLCore::vertexAttribI4i(vertex_abi::IrisEntity, -1, -1, -1, 1);
 }
 core::RenderPass pass;
 pass.modelView = core::drawModelView();
 pass.projection = core::drawProjection();
 core::applyPendingTerrain(pass);
 pass.buffer = arena.handle;
 pass.vertexCount = totalVertices;
 pass.stride = static_cast<int>(kVertexBytes);
 pass.hasTexture = true;
 pass.hasNormals = true;
 return core::submitIndexedQuadsBatch(pass, arena.vao, arena.indexCounts, arena.baseVertices);
}
} // namespace net::minecraft::client::render::chunk
