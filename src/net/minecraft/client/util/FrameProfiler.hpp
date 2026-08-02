#pragma once
#include <chrono>
#include <cstddef>
#include <vector>
#include "net/minecraft/client/util/FramePipeline.hpp"
namespace net::minecraft::client::util {
// Phase-enum stall trace for the run loop; records (phase, duration) per
// frame. Active under MINECRAFT_FRAME_PROFILE, otherwise a no-op.
class FrameProfiler {
 public:
 using Phase = FramePipeline::Phase;
 struct Record {
  Phase phase;
  std::chrono::microseconds duration;
 };
 [[nodiscard]] static FrameProfiler& instance() noexcept {
  static FrameProfiler profiler;
  return profiler;
 }
 void beginFrame();
 void beginPhase(Phase phase);
 void endPhase();
 [[nodiscard]] std::size_t recordCount() const noexcept;
 [[nodiscard]] const std::vector<Record>& records() const noexcept;

 private:
 std::vector<Record> records_;
 Phase currentPhase_ = Phase::Drain;
 std::chrono::steady_clock::time_point phaseStart_{};
};
} // namespace net::minecraft::client::util
