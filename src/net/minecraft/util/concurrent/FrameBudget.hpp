#pragma once
#include <algorithm>
#include <chrono>
namespace net::minecraft::util::concurrent {
// Time-sliced main-thread work: run at least minItems, then stop at deadline.
struct FrameBudget {
 std::chrono::steady_clock::time_point deadline{};
 int minItems = 0;
 [[nodiscard]] static FrameBudget fromMs(int ms, int minItems) {
  return {std::chrono::steady_clock::now() + std::chrono::milliseconds(ms), minItems};
 }
 [[nodiscard]] bool hasRemaining(int done) const noexcept {
  return done < minItems || std::chrono::steady_clock::now() < deadline;
 }
 // Per-frame shared deadline, set once per frame so every drain/upload
 // consumer reads the same remaining budget (see FramePipeline).
 struct Deadline {
  void set(int ms) noexcept {
   point_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
   active_ = true;
  }
  [[nodiscard]] std::chrono::milliseconds remaining() const noexcept {
   const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
   if(now >= point_) {
    return std::chrono::milliseconds(0);
   }
   return std::chrono::duration_cast<std::chrono::milliseconds>(point_ - now);
  }
  [[nodiscard]] bool hasExpired() const noexcept {
   return active_ && std::chrono::steady_clock::now() >= point_;
  }
  [[nodiscard]] bool active() const noexcept {
   return active_;
  }
  [[nodiscard]] std::chrono::steady_clock::time_point point() const noexcept {
   return point_;
  }
  std::chrono::steady_clock::time_point point_{};
  bool active_ = false;
 };
 // Sets the shared per-frame deadline to now + ms and returns it.
 [[nodiscard]] static Deadline& beginFrame(int ms) noexcept {
  Deadline& frame = frameDeadline();
  frame.set(ms);
  return frame;
 }
 // The shared per-frame deadline (read remaining() from the run loop).
 [[nodiscard]] static Deadline& frameDeadline() noexcept {
  static Deadline frameDeadline_;
  return frameDeadline_;
 }
 [[nodiscard]] static FrameBudget fromSharedMs(int ms, int minItems) {
  const auto local = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  const Deadline& frame = frameDeadline();
  if(!frame.active()) {
   return {local, minItems};
  }
  if(!frame.hasExpired()) {
   return {std::min(local, frame.point()), minItems};
  }
  return {local, minItems};
 }
};
} // namespace net::minecraft::util::concurrent
