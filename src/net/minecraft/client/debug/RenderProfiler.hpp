#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace net::minecraft::client::debug {
enum class RenderStage : int {
 FrameSetup = 0,
 FrameDrain,
 Input,
 Ticks,
 RenderOverhead,
 PackPrepare,
 Shadow,
 ShadowComposite,
 Sky,
 Cull,
 Compile,
 SolidTerrain,
 Entities,
 Particles,
 OpaqueDepth,
 TranslucentTerrain,
 Clouds,
 Hand,
 HandDepth,
 Deferred,
 CenterDepth,
 PostProcess,
 HudGui,
 Present,
 Pace,
 Diagnostics,
 EntityCollectCull,
 EntityPreparation,
 EntityOpaque,
 EntityTranslucent,
 BlockEntities,
 EntityDebugLabels,
 ShadowEntityPreparation,
 ShadowEntityDraw,
 TargetClear,
 DepthCaptureCopy,
 CenterDepthSampling,
 DeferredScheduling,
 FramebufferTransitions,
 ModMeshes,
 RenderOrchestration,
 Count
};
constexpr int kRenderStageCount = static_cast<int>(RenderStage::Count);
enum class RenderMetric : int {
 EntityTraversals = 0,
 EntityVisible,
 EntityCulled,
 EntityRendererInvocations,
 GeneratedGeometryBatches,
 GeneratedVertices,
 FilteredGeometryBatches,
 FilteredVertices,
 DrawCalls,
 DrawVertices,
 ProgramBinds,
 TextureBinds,
 FramebufferBinds,
 UniformUploads,
 Allocations,
 RawGlQueries,
 AsyncReadbacks,
 SynchronousReadbacks,
 Copies,
 Count
};
constexpr int kRenderMetricCount = static_cast<int>(RenderMetric::Count);
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
 void record(RenderMetric metric, std::uint64_t count = 1) noexcept;
 void destroy();
 [[nodiscard]] std::vector<std::string> lines() const;
 [[nodiscard]] double cpuAverageNs(RenderStage stage) const noexcept {
  return cpuAvgNs_[static_cast<std::size_t>(stage)];
 }
 [[nodiscard]] double frameAverageNs() const noexcept {
  return frameAvgNs_;
 }
 [[nodiscard]] double metricAverage(RenderMetric metric) const noexcept {
  return metricAvg_[static_cast<std::size_t>(metric)];
 }
 [[nodiscard]] std::uint64_t frameMetric(RenderMetric metric) const noexcept {
  return metrics_[static_cast<std::size_t>(metric)];
 }
 class Scope {
public:
  explicit Scope(RenderStage stage) : stage_(stage) {
   RenderProfiler::instance().beginStage(stage_);
  }
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
  ~Scope() {
   end();
  }
  void end() {
   if(active_) {
    RenderProfiler::instance().endStage(stage_);
    active_ = false;
   }
  }

private:
  RenderStage stage_;
  bool active_ = true;
 };

 private:
 static constexpr int kRingDepth = 4;
 static constexpr std::uint32_t kGpuQueryInterval = 3;
 [[nodiscard]] static const char* stageName(RenderStage stage);
 [[nodiscard]] static const char* metricName(RenderMetric metric);
 void ensureQueries();
 void collectQueries(int slot);
 bool enabled_ = false;
 bool inFrame_ = false;
 bool gpuReady_ = false;
 bool gpuAttempted_ = false;
 bool gpuActiveThisFrame_ = false;
 struct ActiveStage {
  int stage = -1;
  std::int64_t startNs = 0;
  std::int64_t childNs = 0;
  bool queryBegun = false;
 };
 static constexpr int kMaxStageDepth = 32;
 std::array<ActiveStage, kMaxStageDepth> activeStages_{};
 int activeStageDepth_ = 0;
 int ringSlot_ = kRingDepth - 1;
 std::uint32_t frameCounter_ = 0;
 std::int64_t frameStartNs_ = 0;
 std::array<std::int64_t, kRenderStageCount> cpuNs_{};
 std::array<double, kRenderStageCount> cpuAvgNs_{};
 std::array<double, kRenderStageCount> gpuAvgNs_{};
 std::array<std::uint64_t, kRenderMetricCount> metrics_{};
 std::array<double, kRenderMetricCount> metricAvg_{};
 std::int64_t frameNs_ = 0;
 double frameAvgNs_ = 0.0;
 double gpuMeasuredAvgNs_ = 0.0;
 std::array<std::array<unsigned, kRenderStageCount>, kRingDepth> queries_{};
 std::array<std::array<bool, kRenderStageCount>, kRingDepth> queryPending_{};
 mutable std::chrono::steady_clock::time_point lastLinesAt_{};
 mutable std::vector<std::string> linesCache_;
};
} // namespace net::minecraft::client::debug
