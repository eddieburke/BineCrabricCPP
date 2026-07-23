#pragma once
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
};
} // namespace net::minecraft::util::concurrent
