#include "net/minecraft/client/render/pipeline/AsyncDepthSampler.hpp"
#include <cstring>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
namespace net::minecraft::client::render {
namespace {
constexpr unsigned kPixelPackBuffer = 0x88EB;
constexpr unsigned kStreamRead = 0x88E1;
constexpr unsigned kMapReadBit = 0x0001;
constexpr unsigned kSyncGpuCommandsComplete = 0x9117;
constexpr unsigned kAlreadySignaled = 0x911A;
constexpr unsigned kConditionSatisfied = 0x911C;
constexpr unsigned kWaitFailed = 0x911D;
}
AsyncDepthSampler::~AsyncDepthSampler() {
 destroy();
}
bool AsyncDepthSampler::ensureBuffers() {
 gl::GLCore::ensureLoaded();
 if(!gl::GLCore::asyncReadbackSupported) return false;
 for(Slot& slot : slots_) {
  if(slot.buffer != 0) continue;
  gl::GLCore::genBuffers(1, &slot.buffer);
  if(slot.buffer == 0) return false;
  gl::GLCore::bindBuffer(kPixelPackBuffer, slot.buffer);
  gl::GLCore::bufferData(kPixelPackBuffer, static_cast<intptr_t>(sizeof(float)), nullptr, kStreamRead);
 }
 gl::GLCore::bindBuffer(kPixelPackBuffer, 0);
 return true;
}
std::optional<float> AsyncDepthSampler::pollAndIssue(int x, int y) {
 if(x < 0 || y < 0 || !ensureBuffers()) return std::nullopt;
 std::optional<float> result;
 for(std::size_t offset = 0; offset < slots_.size(); ++offset) {
  const std::size_t index = (consumeCursor_ + offset) % slots_.size();
  Slot& slot = slots_[index];
  if(!slot.pending || slot.fence == nullptr) continue;
  const unsigned wait = gl::GLCore::clientWaitSync(slot.fence, 0, 0);
  if(wait == kWaitFailed) {
   gl::GLCore::deleteSync(slot.fence);
   slot.fence = nullptr;
   slot.pending = false;
   consumeCursor_ = (index + 1) % slots_.size();
   continue;
  }
  if(wait != kAlreadySignaled && wait != kConditionSatisfied) break;
  gl::GLCore::bindBuffer(kPixelPackBuffer, slot.buffer);
  void* bytes = gl::GLCore::mapBufferRange(kPixelPackBuffer, 0, static_cast<intptr_t>(sizeof(float)), kMapReadBit);
  if(bytes != nullptr) {
   float depth = 1.0f;
   std::memcpy(&depth, bytes, sizeof(depth));
   result = depth;
   gl::GLCore::unmapBuffer(kPixelPackBuffer);
  }
  gl::GLCore::deleteSync(slot.fence);
  slot.fence = nullptr;
  slot.pending = false;
  consumeCursor_ = (index + 1) % slots_.size();
  break;
 }
 Slot* issue = nullptr;
 for(std::size_t offset = 0; offset < slots_.size(); ++offset) {
  Slot& candidate = slots_[(issueCursor_ + offset) % slots_.size()];
  if(candidate.pending) continue;
  issue = &candidate;
  issueCursor_ = (static_cast<std::size_t>(issue - slots_.data()) + 1) % slots_.size();
  break;
 }
 if(issue != nullptr) {
  gl::GLCore::bindBuffer(kPixelPackBuffer, issue->buffer);
  ::glReadPixels(x, y, 1, 1, 0x1902, 0x1406, nullptr);
  issue->fence = gl::GLCore::fenceSync(kSyncGpuCommandsComplete, 0);
  issue->pending = issue->fence != nullptr;
 }
 gl::GLCore::bindBuffer(kPixelPackBuffer, 0);
 return result;
}
void AsyncDepthSampler::destroy() {
 if(glfwGetCurrentContext() == nullptr) {
  slots_ = {};
  return;
 }
 for(Slot& slot : slots_) {
  if(slot.fence != nullptr && gl::GLCore::deleteSync != nullptr) gl::GLCore::deleteSync(slot.fence);
  if(slot.buffer != 0 && gl::GLCore::deleteBuffers != nullptr) gl::GLCore::deleteBuffers(1, &slot.buffer);
  slot = {};
 }
 issueCursor_ = 0;
 consumeCursor_ = 0;
}
}
