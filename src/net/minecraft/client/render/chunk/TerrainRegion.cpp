#include "net/minecraft/client/render/chunk/TerrainRegion.hpp"
#include <algorithm>
#include <cstring>
#include <optional>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
namespace net::minecraft::client::render::chunk {
namespace {
constexpr unsigned kArrayBuffer = 0x8892;
constexpr unsigned kDynamicDraw = 0x88E8;
constexpr unsigned kDynamicStorageBit = 0x0100;
constexpr std::size_t kMinBlockVertices = 4096;
constexpr std::size_t kMaxBlockVertices = 262144;
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
  for(ArenaBlock& block : arena.blocks) destroyBlock(block);
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
void TerrainRegion::destroyBlock(ArenaBlock& block) noexcept {
 if(block.vao != 0 && gl::GLCore::deleteVertexArrays != nullptr) gl::GLCore::deleteVertexArrays(1, &block.vao);
 if(block.handle != 0 && gl::GLCore::deleteBuffers != nullptr) gl::GLCore::deleteBuffers(1, &block.handle);
 block = {};
}
bool TerrainRegion::createBlock(LayerArena& arena, std::size_t capacityVertices) {
 if(gl::GLCore::genBuffers == nullptr || gl::GLCore::bindBuffer == nullptr ||
    gl::GLCore::deleteBuffers == nullptr || gl::GLCore::genVertexArrays == nullptr ||
    gl::GLCore::deleteVertexArrays == nullptr ||
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
 auto slot = std::find_if(arena.blocks.begin(), arena.blocks.end(),
                          [](const ArenaBlock& block) { return block.handle == 0; });
 if(slot == arena.blocks.end()) slot = arena.blocks.emplace(arena.blocks.end());
 slot->handle = newHandle;
 slot->vao = newVao;
 slot->capacityVertices = capacityVertices;
 slot->freeRanges.emplace(0, capacityVertices);
 return true;
}
bool TerrainRegion::acquire(int layer, std::size_t vertices, TerrainAllocation& allocation) {
 LayerArena& arena = layers_[static_cast<std::size_t>(layer)];
 const std::size_t needed = roundToGranule(vertices);
 const auto findBest = [&arena, needed](std::size_t& blockIndex) {
  std::map<std::size_t, std::size_t>::iterator best{};
  bool found = false;
  for(std::size_t index = 0; index < arena.blocks.size(); ++index) {
   auto& ranges = arena.blocks[index].freeRanges;
   for(auto it = ranges.begin(); it != ranges.end(); ++it) {
    if(it->second >= needed && (!found || it->second < best->second)) {
     best = it;
     blockIndex = index;
     found = true;
    }
   }
  }
  return found ? std::optional(best) : std::nullopt;
 };
 std::size_t blockIndex = 0;
 auto best = findBest(blockIndex);
 if(!best.has_value()) {
  std::size_t total = 0;
  for(const ArenaBlock& block : arena.blocks) total += block.capacityVertices;
  if(!createBlock(arena, std::max(needed, std::clamp(total, kMinBlockVertices, kMaxBlockVertices)))) {
   return false;
  }
  best = findBest(blockIndex);
 }
 ArenaBlock& block = arena.blocks[blockIndex];
 const std::size_t first = (*best)->first;
 const std::size_t length = (*best)->second;
 block.freeRanges.erase(*best);
 if(length > needed) {
  block.freeRanges.emplace(first + needed, length - needed);
 }
 block.liveVertices += needed;
 allocation.firstVertex = first;
 allocation.capacityVertices = needed;
 allocation.vertexCount = 0;
 allocation.block = blockIndex;
 return true;
}
void TerrainRegion::releaseRange(LayerArena& arena, TerrainAllocation& allocation) noexcept {
 if(allocation.capacityVertices == 0) {
  allocation = {};
  return;
 }
 ArenaBlock& block = arena.blocks[allocation.block];
 std::size_t first = allocation.firstVertex;
 std::size_t length = allocation.capacityVertices;
 block.liveVertices -= length;
 allocation = {};
 if(block.liveVertices == 0) {
  destroyBlock(block);
  return;
 }
 auto next = block.freeRanges.lower_bound(first);
 if(next != block.freeRanges.begin()) {
  const auto previous = std::prev(next);
  if(previous->first + previous->second == first) {
   first = previous->first;
   length += previous->second;
   block.freeRanges.erase(previous);
  }
 }
 if(next != block.freeRanges.end() && first + length == next->first) {
  length += next->second;
  block.freeRanges.erase(next);
 }
 block.freeRanges.emplace(first, length);
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
 gl::GLCore::bindBuffer(kArrayBuffer, arena.blocks[allocation.block].handle);
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
 releaseRange(layers_[static_cast<std::size_t>(layer)], allocation);
}
std::size_t TerrainRegion::gpuBytes() const noexcept {
 std::size_t vertices = 0;
 for(const LayerArena& arena : layers_) {
  for(const ArenaBlock& block : arena.blocks) vertices += block.capacityVertices;
 }
 return vertices * kVertexBytes;
}
std::size_t TerrainRegion::liveBytes() const noexcept {
 std::size_t vertices = 0;
 for(const LayerArena& arena : layers_) {
  for(const ArenaBlock& block : arena.blocks) vertices += block.liveVertices;
 }
 return vertices * kVertexBytes;
}
int TerrainRegion::drawLayer(int layer, std::span<const TerrainAllocation* const> allocations) {
 if(layer < 0 || layer >= terrain_layer::Count || allocations.empty()) {
  return 0;
 }
 LayerArena& arena = layers_[static_cast<std::size_t>(layer)];
 int submitted = 0;
 for(std::size_t index = 0; index < arena.blocks.size(); ++index) {
  const ArenaBlock& block = arena.blocks[index];
  if(block.handle == 0 || block.vao == 0) {
   continue;
  }
  std::size_t totalVertices = 0;
  std::size_t maxVertices = 0;
  arena.indexCounts.clear();
  arena.baseVertices.clear();
  for(const TerrainAllocation* allocation : allocations) {
   if(allocation == nullptr || !allocation->valid() || allocation->vertexCount < 4 || allocation->block != index) {
    continue;
   }
   const int quadVertices = allocation->vertexCount - allocation->vertexCount % 4;
   arena.indexCounts.push_back((quadVertices / 4) * 6);
   arena.baseVertices.push_back(static_cast<int>(allocation->firstVertex));
   maxVertices = std::max(maxVertices, static_cast<std::size_t>(quadVertices));
   totalVertices += static_cast<std::size_t>(quadVertices);
  }
  if(arena.indexCounts.empty() || !quad_index::ensure(maxVertices)) {
   continue;
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
  pass.buffer = block.handle;
  pass.vertexCount = totalVertices;
  pass.stride = static_cast<int>(kVertexBytes);
  pass.hasTexture = true;
  pass.hasNormals = true;
  submitted += core::submitIndexedQuadsBatch(pass, block.vao, arena.indexCounts, arena.baseVertices);
 }
 return submitted;
}
} // namespace net::minecraft::client::render::chunk
