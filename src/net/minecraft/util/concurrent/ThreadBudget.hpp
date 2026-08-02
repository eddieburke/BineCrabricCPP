#pragma once
#include <algorithm>
namespace net::minecraft::util::concurrent {
// One global thread budget shared by every domain pool. Computed once at
// ThreadCoordinator::configure; pure math so count sites need no threads.
struct ThreadBudget {
 unsigned cpuBudget = 1;
 unsigned glCompile = 1;
 unsigned io = 2;
 unsigned compute = 1;
 unsigned maxComputeThreads = 8;
 // cpuBudget = max(1, hw - reserved); glCompile = clamp(cpu/4, 1, 2);
 // io = clamp(cpu/4, 2, 3); compute = min(maxComputeThreads, max(1, cpu - glCompile - io)).
 // Worked table (reserved 2, maxComputeThreads 8): 8-core -> cpu 6, glCompile 1,
 // io 2, compute 3; 16-core -> 14/2/3/8; 32-core -> 30/2/3/8 (compute capped).
 [[nodiscard]] static ThreadBudget derive(unsigned hardwareThreads,
                                          unsigned reservedThreads = 2,
                                          unsigned maxComputeThreads = 8) noexcept {
  ThreadBudget budget;
  budget.cpuBudget = std::max(1U, hardwareThreads > reservedThreads ? hardwareThreads - reservedThreads : 0U);
  budget.glCompile = std::clamp(budget.cpuBudget / 4U, 1U, 2U);
  budget.io = std::clamp(budget.cpuBudget / 4U, 2U, 3U);
  budget.maxComputeThreads = std::max(1U, maxComputeThreads);
  // Signed math so a tiny cpuBudget cannot underflow the subtraction.
  const int spare = static_cast<int>(budget.cpuBudget) - static_cast<int>(budget.glCompile) -
                    static_cast<int>(budget.io);
  budget.compute = std::min(budget.maxComputeThreads, static_cast<unsigned>(std::max(1, spare)));
  return budget;
 }
 // Per-owner subdivision while several owners keep private pools (WI-2 interim:
 // mesh/lighting/loader each take compute/3). Sums to <= compute.
 [[nodiscard]] unsigned computeShare(unsigned owners) const noexcept {
  return std::max(1U, compute / std::max(1U, owners));
 }
 // Driver-side worker hint for GL_KHR_parallel_shader_compile (GLCore.cpp).
 [[nodiscard]] unsigned glDriverThreads() const noexcept {
  return std::max(1U, cpuBudget / 4U);
 }
};
} // namespace net::minecraft::util::concurrent
