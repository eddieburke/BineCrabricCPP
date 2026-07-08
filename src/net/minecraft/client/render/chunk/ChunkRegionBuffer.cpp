#include "net/minecraft/client/render/chunk/ChunkRegionBuffer.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
namespace net::minecraft::client::render::chunk {
namespace {
constexpr int kStride = static_cast<int>(sizeof(TessellatorVertex));
constexpr int kSplitSlack = 64;
constexpr std::size_t kInitialVertices = 4096;
const void* attribOffset(std::size_t byteOffset) noexcept {
  return reinterpret_cast<const void*>(byteOffset);
}
constexpr unsigned kArrayBuffer = 0x8892;
constexpr unsigned kStaticDraw = 0x88E4;
} // namespace
ChunkRegionBuffer::~ChunkRegionBuffer() {
  if(handle_ != 0) {
    gl::GLCore::deleteBuffers(1, &handle_);
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
                         static_cast<std::ptrdiff_t>(gpuCapacity_ * static_cast<std::size_t>(kStride)), nullptr,
                         kStaticDraw);
  if(!shadow_.empty()) {
    gl::GLCore::bufferSubData(kArrayBuffer, 0,
                              static_cast<std::ptrdiff_t>(shadow_.size() * static_cast<std::size_t>(kStride)),
                              shadow_.data());
  }
}
void ChunkRegionBuffer::upload(Slot& slot, const TessellatorVertex* data, int count, bool hasTexture, bool hasColor,
                               bool hasNormals) {
  if(count <= 0 || data == nullptr) {
    release(slot);
    return;
  }
  const bool nextHasTexture = hasTexture;
  const bool nextHasColor = hasColor;
  const bool nextHasNormals = hasNormals;
  if(layoutSet_ &&
     (hasTexture_ != nextHasTexture || hasColor_ != nextHasColor || hasNormals_ != nextHasNormals)) {
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
  freeList_.push_back(Slot{slot.offset, slot.capacity, 0});
  slot = Slot{};
  std::sort(freeList_.begin(), freeList_.end(),
            [](const Slot& a, const Slot& b) noexcept { return a.offset < b.offset; });
  std::size_t write = 0;
  for(std::size_t read = 0; read < freeList_.size(); ++read) {
    if(write > 0 && freeList_[write - 1].offset + freeList_[write - 1].capacity == freeList_[read].offset) {
      freeList_[write - 1].capacity += freeList_[read].capacity;
    } else {
      freeList_[write++] = freeList_[read];
    }
  }
  freeList_.resize(write);
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
  firsts_.clear();
  counts_.clear();
}
void ChunkRegionBuffer::addVisible(const Slot& slot) {
  if(slot.count <= 0) {
    return;
  }
  firsts_.push_back(slot.offset);
  counts_.push_back(slot.count);
}
int ChunkRegionBuffer::flush(int mode) {
  if(firsts_.empty() || handle_ == 0) {
    return 0;
  }
  gl::GLCore::bindBuffer(kArrayBuffer, handle_);
  const gl::ClientArrayBind arrays(attribOffset(offsetof(TessellatorVertex, x)),
                                   attribOffset(offsetof(TessellatorVertex, u)),
                                   attribOffset(offsetof(TessellatorVertex, color)),
                                   attribOffset(offsetof(TessellatorVertex, normal)), kStride, hasTexture_, hasColor_,
                                   hasNormals_);
  for(std::size_t i = 0; i < firsts_.size(); ++i) {
    gl::drawArrays(mode, firsts_[i], counts_[i]);
  }
  gl::GLCore::bindBuffer(kArrayBuffer, 0);
  return static_cast<int>(firsts_.size());
}
} // namespace net::minecraft::client::render::chunk
