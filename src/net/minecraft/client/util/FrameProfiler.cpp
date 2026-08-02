#include "net/minecraft/client/util/FrameProfiler.hpp"
namespace net::minecraft::client::util {
void FrameProfiler::beginFrame() {
#ifdef MINECRAFT_FRAME_PROFILE
  records_.clear();
#endif
}
void FrameProfiler::beginPhase(Phase phase) {
#ifdef MINECRAFT_FRAME_PROFILE
  currentPhase_ = phase;
  phaseStart_ = std::chrono::steady_clock::now();
#else
  (void)phase;
#endif
}
void FrameProfiler::endPhase() {
#ifdef MINECRAFT_FRAME_PROFILE
  const std::chrono::microseconds duration =
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - phaseStart_);
  records_.push_back({currentPhase_, duration});
#endif
}
std::size_t FrameProfiler::recordCount() const noexcept {
 return records_.size();
}
const std::vector<FrameProfiler::Record>& FrameProfiler::records() const noexcept {
 return records_;
}
} // namespace net::minecraft::client::util
