#include "net/minecraft/client/debug/PerformanceCapture.hpp"
#include <algorithm>
#include <chrono>
#include <utility>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/debug/PerformanceReport.hpp"
#include "net/minecraft/client/debug/RenderProfiler.hpp"
namespace net::minecraft::client::debug {
namespace {
double milliseconds(double nanos) {
 return nanos / 1.0e6;
}
} // namespace
void PerformanceCapture::configure(PerformanceCaptureConfig config) {
 config.warmupFrames = std::max(0, config.warmupFrames);
 config.captureFrames = std::max(0, config.captureFrames);
 config.captureSeconds = std::max(0, config.captureSeconds);
 if(config.captureFrames == 0 && config.captureSeconds == 0) {
  config.captureFrames = 600;
 }
 config.progressIntervalFrames = std::max(1, config.progressIntervalFrames);
 config_ = std::move(config);
 warmedFrames_ = 0;
 capturedFrames_ = 0;
 stopRequested_ = false;
 state_ = config_.enabled ? State::Waiting : State::Disabled;
}
bool PerformanceCapture::beginFrame(bool renderReady, RenderProfiler& profiler) {
 if(state_ == State::Disabled || state_ == State::Complete) {
  return false;
 }
 if(state_ == State::Waiting && renderReady) {
  state_ = config_.warmupFrames == 0 ? State::Capturing : State::Warmup;
  const std::string captureLength = config_.captureFrames > 0
                                        ? std::to_string(config_.captureFrames) + " frames"
                                        : std::to_string(config_.captureSeconds) + " seconds";
  ClientLog::LOGGER.info("[perf] render ready; warmup=" + std::to_string(config_.warmupFrames) +
                         " frames, capture=" + captureLength);
  if(state_ == State::Capturing) {
   startCapture(profiler);
  }
 }
 if(state_ == State::Warmup && warmedFrames_ >= config_.warmupFrames) {
  startCapture(profiler);
 }
 return state_ == State::Warmup || state_ == State::Capturing;
}
void PerformanceCapture::startCapture(RenderProfiler& profiler) {
 profiler.resetSamples();
 loggingStart_ = net::minecraft::util::logging::LogDispatcher::instance().stats();
 state_ = State::Capturing;
 capturedFrames_ = 0;
 captureStart_ = std::chrono::steady_clock::now();
 ClientLog::LOGGER.info("[perf] capture started");
}
void PerformanceCapture::endFrame(RenderProfiler& profiler) {
 if(state_ == State::Warmup) {
  ++warmedFrames_;
  return;
 }
 if(state_ != State::Capturing) {
  return;
 }
 ++capturedFrames_;
 const bool frameComplete = config_.captureFrames > 0 && capturedFrames_ >= config_.captureFrames;
 const bool timeComplete = config_.captureSeconds > 0 &&
                           std::chrono::steady_clock::now() - captureStart_ >= std::chrono::seconds(config_.captureSeconds);
 if(capturedFrames_ % config_.progressIntervalFrames == 0 && !frameComplete && !timeComplete) {
  const RenderProfileSnapshot sample = profiler.snapshot();
  ClientLog::LOGGER.info("[perf] captured " + std::to_string(capturedFrames_) + "/" +
                         (config_.captureFrames > 0 ? std::to_string(config_.captureFrames) : std::string("time")) +
                         " frames; mean=" +
                         std::to_string(milliseconds(sample.frameAverageNs)) + " ms, p95=" +
                         std::to_string(milliseconds(sample.frameP95Ns)) + " ms");
 }
 if(frameComplete || timeComplete) {
  finishCapture(profiler);
 }
}
void PerformanceCapture::finishCapture(const RenderProfiler& profiler) {
 const bool written = writeReport(profiler);
 const RenderProfileSnapshot sample = profiler.snapshot();
 ClientLog::LOGGER.info("[perf] capture complete; frames=" + std::to_string(sample.frames) +
                        ", mean=" + std::to_string(milliseconds(sample.frameAverageNs)) +
                        " ms, p95=" + std::to_string(milliseconds(sample.frameP95Ns)) +
                        " ms, report=" + (written ? config_.output.string() : std::string("write failed")));
 state_ = State::Complete;
 stopRequested_ = config_.autoStop;
}
bool PerformanceCapture::writeReport(const RenderProfiler& profiler) const {
 PerformanceReportMetadata metadata;
 metadata.shaderPack = config_.shaderPack;
 metadata.world = config_.world;
 metadata.width = config_.width;
 metadata.height = config_.height;
 metadata.warmupFrames = config_.warmupFrames;
 metadata.captureSeconds = config_.captureSeconds;
 return writePerformanceReport(config_.output, metadata, profiler.snapshot(), loggingStart_,
                               net::minecraft::util::logging::LogDispatcher::instance().stats());
}
bool PerformanceCapture::enabled() const noexcept {
 return state_ != State::Disabled;
}
bool PerformanceCapture::complete() const noexcept {
 return state_ == State::Complete;
}
bool PerformanceCapture::shouldStop() const noexcept {
 return stopRequested_;
}
int PerformanceCapture::capturedFrames() const noexcept {
 return capturedFrames_;
}
const std::filesystem::path& PerformanceCapture::outputPath() const noexcept {
 return config_.output;
}
} // namespace net::minecraft::client::debug
