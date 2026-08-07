#pragma once
#include <chrono>
#include <cstddef>
#include <functional>
#include <vector>
#include "net/minecraft/client/core/TaskMailbox.hpp"
namespace net::minecraft::client::util {
class FramePipeline {
 public:
  enum class Phase { Drain,
                     Input,
                     Ticks,
                     Render,
                     Present,
                     Pace,
                     Diagnostics };
  static constexpr Phase kPhaseOrder[] = {Phase::Drain, Phase::Input, Phase::Ticks, Phase::Render,
                                          Phase::Present, Phase::Pace, Phase::Diagnostics};
 static constexpr std::size_t kPhaseCount = sizeof(kPhaseOrder) / sizeof(kPhaseOrder[0]);
 [[nodiscard]] static constexpr Phase phaseAt(std::size_t index) noexcept {
  return kPhaseOrder[index];
 }
 [[nodiscard]] static constexpr std::size_t count() noexcept {
  return kPhaseCount;
 }
 struct Record {
  Phase phase;
  std::chrono::microseconds duration;
 };
 void beginFrame();
 void beginPhase(Phase phase);
 void endPhase();
 [[nodiscard]] std::size_t recordCount() const noexcept;
 [[nodiscard]] const std::vector<Record>& records() const noexcept;
 using PhaseTask = std::function<void(Phase)>;
 void run();
 void run(const PhaseTask& task);
 [[nodiscard]] core::TaskMailbox& mailbox() noexcept {
  return mailbox_;
 }

 private:
 core::TaskMailbox mailbox_;
 std::vector<Record> records_;
 Phase currentPhase_ = Phase::Drain;
 std::chrono::steady_clock::time_point phaseStart_{};
};
} // namespace net::minecraft::client::util
