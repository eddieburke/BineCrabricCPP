#include "net/minecraft/client/render/chunk/ChunkRegionBuffer.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::render::chunk {
namespace {
constexpr int kStride = static_cast<int>(sizeof(TessellatorVertex));
constexpr int kSplitSlack = 64;
constexpr std::size_t kInitialVertices = 4096;
constexpr unsigned kArrayBuffer = 0x8892;
constexpr unsigned kDynamicDraw = 0x88E8;
} // namespace
ChunkRegionBuffer::~ChunkRegionBuffer() {
 if(handle_ != 0) {
  gl::GLCore::deleteBuffers(1, &handle_);
  gl::engine_pipeline::invalidateAttribCache();
  handle_ = 0;
 }
}
ChunkRegionBuffer::Slot ChunkRegionBuffer::allocate(int count) {
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
void ChunkRegionBuffer::upload(
    Slot& slot, const TessellatorVertex* data, int count, bool hasTexture, bool hasColor, bool hasNormals) {
 if(count <= 0 || data == nullptr) {
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
 std::copy(data, data + count, shadow_.begin() + slot.offset);
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
void ChunkRegionBuffer::beginFrame() noexcept {
 visible_.clear();
 merged_.clear();
}
void ChunkRegionBuffer::addVisible(const Slot& slot) {
 if(slot.count <= 0) {
  return;
 }
 visible_.push_back(DrawRange{slot.offset, slot.count});
}
// Sorts the frame's visible ranges by buffer offset and coalesces any that are
// exactly adjacent into one. Sections are allocated in contiguous runs (see
// allocate()/release()), so neighbouring sections frequently share a run and a
// few hundred draw calls collapse into a few dozen.
//
// This is arithmetic only and cannot change the rendered result: the shared quad
// index buffer is a positional 0,1,2,0,2,3 repeat over the whole vertex buffer,
// so the indices for [a, a+n) followed by [a+n, a+n+m) are exactly the indices
// for [a, a+n+m). Both offsets and counts are whole quads (multiples of 4)
// because every upload comes from a quad-mode mesh; ranges that are not are
// left unmerged rather than trusted.
void ChunkRegionBuffer::buildMergedRanges() {
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
  if(!gl::engine_pipeline::ensureReady()) {
   return 0;
  }
 gl::engine_pipeline::bindAndUploadUniforms();
  if(gl::engine_pipeline::program() == nullptr) {
   return 0;
  }
 gl::engine_pipeline::configureAttribs(handle_, 0, kStride, hasTexture_, hasColor_, hasNormals_);
 if(!render::quad_index::ensure(shadow_.size())) {
  return 0;
 }
 buildMergedRanges();
 gl::GLCore::bindBuffer(0x8893, render::quad_index::handle());
 for(const DrawRange& range : merged_) {
  const int indexCount = (range.count / 4) * 6;
  const std::size_t indexByteOffset = (static_cast<std::size_t>(range.first) / 4) * 6 * sizeof(std::uint32_t);
  ::glDrawElements(0x0004, indexCount, 0x1405, reinterpret_cast<const void*>(indexByteOffset));
 }
 frameVisibleRanges += static_cast<int>(visible_.size());
 frameDrawCalls += static_cast<int>(merged_.size());
 return static_cast<int>(merged_.size());
}
} // namespace net::minecraft::client::render::chunk
