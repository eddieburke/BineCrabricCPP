#pragma once
#include <algorithm>
#include <chrono>
namespace net::minecraft::util::concurrent {
// A bounded slice of main-thread work. Every slice runs at least minItems items
// unconditionally, then stops once its wall-clock deadline passes. All frame-loop
// consumers (chunk mesh upload, chunk-column adoption, pending-column drain)
// share ONE per-frame deadline so a slow slice cannot stretch a single frame far
// past budget; local slices are derived from it via fromSharedMs().
class FrameBudget {
 public:
  // Local slice that ignores the shared frame deadline: now + ms, at least
  // minItems.
  [[nodiscard]] static FrameBudget fromMs(int ms, int minItems) noexcept {
   return FrameBudget{std::chrono::steady_clock::now() + std::chrono::milliseconds(ms), minItems};
  }
  // Slice bounded by the shared frame deadline: at least minItems, then stop at
  // the earlier of now + ms and the frame deadline. If no frame deadline is
  // armed or it has already expired (this frame ran over budget), use a fresh
  // now + ms slice so work keeps making progress instead of stalling at zero.
  [[nodiscard]] static FrameBudget fromSharedMs(int ms, int minItems) noexcept {
   const FrameBudget& frame = frameDeadline();
   if(!frame.active()) {
    return fromMs(ms, minItems);
   }
   if(frame.expired()) {
    return fromMs(ms, minItems);
   }
   const auto local = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
   return FrameBudget{std::min(local, frame.deadline()), minItems};
  }
  // True while at least minItems are not yet done, or the deadline has not passed.
  [[nodiscard]] bool hasRemaining(int done) const noexcept {
   return done < minItems_ || std::chrono::steady_clock::now() < deadline_;
  }
  // True once the deadline has passed.
  [[nodiscard]] bool expired() const noexcept {
   return std::chrono::steady_clock::now() >= deadline_;
  }
  // True once a deadline is armed (only the shared frame deadline can be unarmed).
  [[nodiscard]] bool active() const noexcept {
   return deadline_ != std::chrono::steady_clock::time_point{};
  }
  // Wall time until the deadline (zero once passed).
  [[nodiscard]] std::chrono::nanoseconds remaining() const noexcept {
   const auto now = std::chrono::steady_clock::now();
   if(now >= deadline_) {
    return std::chrono::nanoseconds(0);
   }
   return std::chrono::duration_cast<std::chrono::nanoseconds>(deadline_ - now);
  }

  // The one shared per-frame deadline. Inactive until beginFrame() runs.
  [[nodiscard]] static const FrameBudget& frameDeadline() noexcept {
   return frameDeadlineMutable();
  }
  // Arms the shared per-frame deadline to now + ms. Called once per frame by
  // FramePipeline before any consumer slices are derived.
  static void beginFrame(int ms) noexcept {
   frameDeadlineMutable() = fromMs(ms, 0);
  }

 private:
  FrameBudget() = default;
  FrameBudget(std::chrono::steady_clock::time_point deadline, int minItems)
      : deadline_(deadline), minItems_(minItems) {
  }
  [[nodiscard]] std::chrono::steady_clock::time_point deadline() const noexcept {
   return deadline_;
  }
  [[nodiscard]] static FrameBudget& frameDeadlineMutable() noexcept {
   static FrameBudget frame;
   return frame;
  }
  std::chrono::steady_clock::time_point deadline_{};
  int minItems_ = 0;
};
} // namespace net::minecraft::util::concurrent
