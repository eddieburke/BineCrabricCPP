#include "net/minecraft/client/render/chunk/ChunkRegionBuffer.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::client::render::chunk {
namespace math = net::minecraft::util::math;
namespace {
constexpr int kStride = static_cast<int>(sizeof(TessellatorVertex));
constexpr int kSplitSlack = 64;
constexpr std::size_t kInitialVertices = 1u << 16;
constexpr unsigned kArrayBuffer = 0x8892;
constexpr unsigned kDynamicDraw = 0x88E8;
} // namespace
ChunkRegionBuffer::~ChunkRegionBuffer() {
 if(handle_ != 0) {
  gl::GLCore::deleteBuffers(1, &handle_);
  render::core::invalidateAttribCache();
  handle_ = 0;
 }
}
// Every slot offset and capacity stays a multiple of 4 vertices. flush() draws
// through the shared quad index buffer, which addresses whole quads, so
// a misaligned range would draw a quad straddling two sections. The invariant
// holds by induction from `count` always being a quad multiple — terrain only
// ever emits through startQuads() — and this asserts the base case rather than
// leaving it as folklore. See upload().
ChunkRegionBuffer::Slot ChunkRegionBuffer::allocate(int count) {
 assert(count % 4 == 0 && "ChunkRegionBuffer slots must be whole quads");
 for(std::size_t i = 0; i < freeList_.size(); ++i) {
  Slot free = freeList_[i];
  if(free.capacity < count) {
   continue;
  }
  if(free.capacity - count <= kSplitSlack) {
   freeList_.erase(freeList_.begin() + static_cast<std::ptrdiff_t>(i));
   return Slot{free.offset, free.capacity, 0};
  }
  freeList_[i] = Slot{free.offset + count, free.capacity - count, 0};
  return Slot{free.offset, count, 0};
 }
 const int offset = static_cast<int>(shadow_.size());
 shadow_.resize(shadow_.size() + static_cast<std::size_t>(count));
 return Slot{offset, count, 0};
}
void ChunkRegionBuffer::reallocBuffer(std::size_t newCapacityVertices) {
 gpuCapacity_ = std::max({newCapacityVertices, gpuCapacity_ * 2, kInitialVertices});
 gl::GLCore::bufferData(kArrayBuffer,
                        static_cast<std::ptrdiff_t>(gpuCapacity_ * static_cast<std::size_t>(kStride)),
                        nullptr,
                        kDynamicDraw);
 if(!shadow_.empty()) {
  gl::GLCore::bufferSubData(kArrayBuffer,
                            0,
                            static_cast<std::ptrdiff_t>(shadow_.size() * static_cast<std::size_t>(kStride)),
                            shadow_.data());
 }
}
void ChunkRegionBuffer::reserve(std::size_t vertexCapacity) {
 if(vertexCapacity <= gpuCapacity_) {
  return;
 }
 if(handle_ == 0) {
  gl::GLCore::genBuffers(1, &handle_);
 }
 gl::GLCore::bindBuffer(kArrayBuffer, handle_);
 reallocBuffer(vertexCapacity);
 gl::GLCore::bindBuffer(kArrayBuffer, 0);
}
void ChunkRegionBuffer::upload(Slot& slot,
                               const TessellatorVertex* data,
                               int count,
                               bool hasTexture,
                               bool hasColor,
                               bool hasNormals,
                               float bakeX,
                               float bakeY,
                               float bakeZ) {
 if(count <= 0 || data == nullptr) {
  release(slot);
  return;
 }
 // Whole quads only. A partial quad cannot be drawn by either path in flush()
 // and used to be silently truncated there; refusing it here keeps the failure
 // at the producer instead of three vertices vanishing from the world.
 assert(count % 4 == 0 && "ChunkRegionBuffer::upload expects whole quads");
 count -= count % 4;
 if(count == 0) {
  release(slot);
  return;
 }
 const bool nextHasTexture = hasTexture;
 const bool nextHasColor = hasColor;
 const bool nextHasNormals = hasNormals;
 if(layoutSet_ && (hasTexture_ != nextHasTexture || hasColor_ != nextHasColor || hasNormals_ != nextHasNormals)) {
  layoutSet_ = false;
 }
 if(!layoutSet_) {
  hasTexture_ = nextHasTexture;
  hasColor_ = nextHasColor;
  hasNormals_ = nextHasNormals;
  layoutSet_ = true;
 }
 if(!(slot.valid() && slot.capacity >= count)) {
  release(slot);
  slot = allocate(count);
 }
 slot.count = count;
 TessellatorVertex* dst = shadow_.data() + slot.offset;
 for(int i = 0; i < count; ++i) {
  dst[i] = data[i];
  dst[i].x += bakeX;
  dst[i].y += bakeY;
  dst[i].z += bakeZ;
 }
 if(handle_ == 0) {
  gl::GLCore::genBuffers(1, &handle_);
 }
 gl::GLCore::bindBuffer(kArrayBuffer, handle_);
 if(gpuCapacity_ < shadow_.size()) {
  reallocBuffer(shadow_.size());
 } else {
  gl::GLCore::bufferSubData(kArrayBuffer,
                            static_cast<std::ptrdiff_t>(static_cast<std::size_t>(slot.offset) * kStride),
                            static_cast<std::ptrdiff_t>(static_cast<std::size_t>(count) * kStride),
                            &shadow_[static_cast<std::size_t>(slot.offset)]);
 }
 gl::GLCore::bindBuffer(kArrayBuffer, 0);
}
void ChunkRegionBuffer::release(Slot& slot) noexcept {
 if(!slot.valid()) {
  slot = Slot{};
  return;
 }
 const Slot freed{slot.offset, slot.capacity, 0};
 slot = Slot{};
 const auto pos =
     std::lower_bound(freeList_.begin(), freeList_.end(), freed, [](const Slot& a, const Slot& b) noexcept {
      return a.offset < b.offset;
     });
 std::size_t idx = static_cast<std::size_t>(pos - freeList_.begin());
 freeList_.insert(pos, freed);
 if(idx + 1 < freeList_.size() && freeList_[idx].offset + freeList_[idx].capacity == freeList_[idx + 1].offset) {
  freeList_[idx].capacity += freeList_[idx + 1].capacity;
  freeList_.erase(freeList_.begin() + static_cast<std::ptrdiff_t>(idx + 1));
 }
 if(idx > 0 && freeList_[idx - 1].offset + freeList_[idx - 1].capacity == freeList_[idx].offset) {
  freeList_[idx - 1].capacity += freeList_[idx].capacity;
  freeList_.erase(freeList_.begin() + static_cast<std::ptrdiff_t>(idx));
 }
 while(!freeList_.empty()) {
  const Slot& last = freeList_.back();
  if(static_cast<std::size_t>(last.offset + last.capacity) != shadow_.size()) {
   break;
  }
  shadow_.resize(static_cast<std::size_t>(last.offset));
  freeList_.pop_back();
 }
}
void ChunkRegionBuffer::beginFrame(float chunkOffsetX, float chunkOffsetY, float chunkOffsetZ) noexcept {
 frameChunkOffset_[0] = chunkOffsetX;
 frameChunkOffset_[1] = chunkOffsetY;
 frameChunkOffset_[2] = chunkOffsetZ;
 visible_.clear();
}
void ChunkRegionBuffer::addVisible(const Slot& slot) {
 if(slot.count <= 0) {
  return;
 }
 visible_.push_back(DrawRange{slot.offset, slot.count});
}
void ChunkRegionBuffer::buildMergedRanges() {
 if(visible_.size() == lastVisible_.size() &&
    std::equal(visible_.begin(), visible_.end(), lastVisible_.begin(), [](const DrawRange& a, const DrawRange& b) {
     return a.first == b.first && a.count == b.count;
    })) {
  return;
 }
 lastVisible_ = visible_;
 merged_.clear();
 if(visible_.empty()) {
  return;
 }
 std::sort(visible_.begin(), visible_.end(), [](const DrawRange& a, const DrawRange& b) noexcept {
  return a.first < b.first;
 });
 merged_.push_back(visible_.front());
 for(std::size_t i = 1; i < visible_.size(); ++i) {
  DrawRange& tail = merged_.back();
  const DrawRange& next = visible_[i];
  const bool quadAligned = (tail.first % 4 == 0) && (tail.count % 4 == 0) && (next.first % 4 == 0);
  if(quadAligned && tail.first + tail.count == next.first) {
   tail.count += next.count;
   continue;
  }
  merged_.push_back(next);
 }
}
int ChunkRegionBuffer::flush() {
 if(visible_.empty() || handle_ == 0) {
  return 0;
 }
 gl::GLCore::bindBuffer(kArrayBuffer, handle_);
 if(!render::core::ensureReady() || render::core::program() == nullptr) {
  return 0;
 }
 render::core::configureAttribs(handle_, 0, kStride, hasTexture_, hasColor_, hasNormals_);
 buildMergedRanges();
 if(merged_.empty()) {
  return 0;
 }
 // Terrain is stored region-local, so every range in this buffer shares one
 // chunkOffset (region origin - camera): one uniform upload, then all ranges in
 // a single multi-draw call.
 render::core::RenderPass pass;
 pass.modelView = render::core::drawModelView();
 pass.projection = render::core::drawProjection();
 pass.fog = render::core::fog();
 pass.hasTexture = hasTexture_;
 pass.hasColor = hasColor_;
 pass.hasNormals = hasNormals_;
 pass.sectionLocal = true;
 pass.chunkOffset[0] = frameChunkOffset_[0];
 pass.chunkOffset[1] = frameChunkOffset_[1];
 pass.chunkOffset[2] = frameChunkOffset_[2];
 if(!hasTexture_) {
  render::core::bindWhiteDiffuse();
 }
 render::core::bindAndUploadUniforms(pass);
 // Terrain stores quads, and the context is core-profile forward-compatible
 // (Window.cpp asks for GLFW_OPENGL_CORE_PROFILE), where GL_QUADS does not exist:
 // a glDrawArrays(GL_QUADS) here raised GL_INVALID_ENUM and drew nothing, which is
 // why terrain vanished while every triangle producer kept rendering. Quads reach
 // the GPU the one way they can, through the shared quad index buffer — the same
 // route Tessellator::draw takes for its own quad batches.
 if(!render::quad_index::ensure(shadow_.size())) {
  return 0;
 }
 gl::GLCore::bindBuffer(0x8893, render::quad_index::handle());
 int draws = 0;
 for(const DrawRange& range : merged_) {
  // The shared quad index buffer addresses whole quads, so a range that is not
  // quad-aligned cannot be expressed here at all. upload()/allocate() guarantee
  // it is; assert rather than silently dropping the remainder, which is what the
  // old `(count / 4) * 6` truncation did.
  assert(range.first % 4 == 0 && range.count % 4 == 0 && "unaligned terrain draw range");
  const int indexCount = (range.count / 4) * 6;
  const std::size_t indexByteOffset = (static_cast<std::size_t>(range.first) / 4) * 6 * sizeof(std::uint32_t);
  ::glDrawElements(0x0004, indexCount, 0x1405, reinterpret_cast<const void*>(indexByteOffset));
  ++draws;
 }
 frameVisibleRanges += static_cast<int>(visible_.size());
 frameDrawCalls += draws;
 // TEMP DIAGNOSTIC — [terrain-probe]; see ChunkRegionBuffer.hpp probeArmed.
 if(probeArmed) {
  ++probeRegions;
  probeRanges += static_cast<int>(merged_.size());
  for(const DrawRange& range : merged_) {
   probeVertices += range.count;
   // Sample: the corners of a range bound it closely enough to see a section
   // sitting on the camera or flung a region away.
   for(int i = 0; i < range.count; i += 37) {
    const TessellatorVertex& vertex = shadow_[static_cast<std::size_t>(range.first + i)];
    const float world[3] = {vertex.x + frameChunkOffset_[0],
                            vertex.y + frameChunkOffset_[1],
                            vertex.z + frameChunkOffset_[2]};
    for(int axis = 0; axis < 3; ++axis) {
     if(!probeAny || world[axis] < probeMin[axis]) probeMin[axis] = world[axis];
     if(!probeAny || world[axis] > probeMax[axis]) probeMax[axis] = world[axis];
    }
    probeAny = true;
   }
  }
  const math::Matrix4f& sentProjection = render::core::uploadedProjection();
  const math::Matrix4f& sentModelView = render::core::uploadedModelView();
  std::copy(std::begin(sentProjection.m), std::end(sentProjection.m), std::begin(probeProjection));
  std::copy(std::begin(sentModelView.m), std::end(sentModelView.m), std::begin(probeModelView));
 }
 return draws;
}
ChunkRegion& ChunkRegionManager::regionFor(int sectionX, int sectionY, int sectionZ) {
 const RegionKey key{sectionX >> 3, sectionY >> 2, sectionZ >> 3};
 auto it = regions_.find(key);
 if(it != regions_.end()) {
  return *it->second;
 }
 auto region = std::make_unique<ChunkRegion>();
 if(reserveHint_ > 0) {
  for(auto& layer : region->layers) layer.reserve(reserveHint_);
 }
 auto [pos, inserted] = regions_.emplace(key, std::move(region));
 return *pos->second;
}
void ChunkRegionManager::releaseRegion(const RegionKey& key) {
 auto it = regions_.find(key);
 if(it == regions_.end()) {
  return;
 }
 ChunkRegion& region = *it->second;
 if(region.sectionCount > 0) {
  --region.sectionCount;
 }
 if(region.sectionCount <= 0) {
  regions_.erase(it);
 }
}
void ChunkRegionManager::clear() noexcept {
 regions_.clear();
}
} // namespace net::minecraft::client::render::chunk
