#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace net::minecraft::client::debug {
enum class RenderStage : int {
 Sky = 0,
 Cull,
 Compile,
 SolidTerrain,
 Entities,
 Particles,
 TranslucentTerrain,
 Clouds,
 Hand,
 Count
};
constexpr int kRenderStageCount = static_cast<int>(RenderStage::Count);
class RenderProfiler {
 public:
 static RenderProfiler& instance();
 void setEnabled(bool enabled);
 [[nodiscard]] bool enabled() const noexcept {
  return enabled_;
 }
 void beginFrame();
 void endFrame();
 void beginStage(RenderStage stage);
 void endStage(RenderStage stage);
 void destroy();
 [[nodiscard]] std::vector<std::string> lines() const;
 class Scope {
public:
  explicit Scope(RenderStage stage) : stage_(stage) {
   RenderProfiler::instance().beginStage(stage_);
  }
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
  ~Scope() {
   RenderProfiler::instance().endStage(stage_);
  }

private:
  RenderStage stage_;
 };

 private:
 static constexpr int kRingDepth = 4;
 [[nodiscard]] static const char* stageName(RenderStage stage);
 void ensureQueries();
 void collectQueries();
 bool enabled_ = false;
 bool inFrame_ = false;
 bool gpuReady_ = false;
 bool gpuAttempted_ = false;
 int activeStage_ = -1;
 bool activeQueryBegun_ = false;
 int ringSlot_ = 0;
 std::int64_t stageStartNs_ = 0;
 std::int64_t frameStartNs_ = 0;
 std::array<std::int64_t, kRenderStageCount> cpuNs_{};
 std::array<double, kRenderStageCount> cpuAvgNs_{};
 std::array<double, kRenderStageCount> gpuAvgNs_{};
 std::int64_t frameNs_ = 0;
 double frameAvgNs_ = 0.0;
 double gpuFrameAvgNs_ = 0.0;
 std::array<std::array<unsigned, kRenderStageCount>, kRingDepth> queries_{};
 std::array<std::array<bool, kRenderStageCount>, kRingDepth> queryPending_{};
};
} // namespace net::minecraft::client::debug
