#pragma once
#include <chrono>
#include <filesystem>
#include <string>
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::client::debug {
class RenderProfiler;
struct PerformanceCaptureConfig {
 bool enabled = false;
 bool autoStop = false;
 int warmupFrames = 120;
 int captureFrames = 600;
 int captureSeconds = 0;
 int progressIntervalFrames = 120;
 int width = 0;
 int height = 0;
 std::filesystem::path output;
 std::string shaderPack;
 std::string world;
};
class PerformanceCapture {
 public:
 void configure(PerformanceCaptureConfig config);
 [[nodiscard]] bool beginFrame(bool renderReady, RenderProfiler& profiler);
 void endFrame(RenderProfiler& profiler);
 [[nodiscard]] bool enabled() const noexcept;
 [[nodiscard]] bool complete() const noexcept;
 [[nodiscard]] bool shouldStop() const noexcept;
 [[nodiscard]] int capturedFrames() const noexcept;
 [[nodiscard]] const std::filesystem::path& outputPath() const noexcept;

 private:
 enum class State { Disabled,
                    Waiting,
                    Warmup,
                    Capturing,
                    Complete };
 void startCapture(RenderProfiler& profiler);
 void finishCapture(const RenderProfiler& profiler);
 bool writeReport(const RenderProfiler& profiler) const;
 PerformanceCaptureConfig config_{};
 State state_ = State::Disabled;
 int warmedFrames_ = 0;
 int capturedFrames_ = 0;
 bool stopRequested_ = false;
 net::minecraft::util::logging::LogDispatcherStats loggingStart_{};
 std::chrono::steady_clock::time_point captureStart_{};
};
} // namespace net::minecraft::client::debug
