#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace net::minecraft::client::debug {
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
 ArenaGrows,
 ArenaGrowVertices,
 QuadIndexGrows,
 DrawSubBatches,
 ChunkColumnsPending,
 ChunkSectionsResident,
 ChunkSectionsVisible,
 MeshJobsQueued,
 MeshJobsInFlight,
 MeshUploadsPending,
 MeshUploadBytes,
 ShadowSectionsVisible,
 ShaderPasses,
 ComputeDispatches,
 ComputeWorkgroups,
 MemoryBarriers,
 MipmapGenerations,
 ShadowDepthCopies,
 RenderTargetAllocations,
 Count
};
constexpr int kRenderMetricCount = static_cast<int>(RenderMetric::Count);
struct RenderSpanSnapshot {
 std::string name;
 double cpuAverageNs = 0.0;
 double gpuAverageNs = 0.0;
 std::uint64_t cpuSamples = 0;
 std::uint64_t gpuSamples = 0;
};
struct RenderProfileSnapshot {
 std::uint64_t frames = 0;
 std::uint64_t gpuFrames = 0;
 std::size_t frameHistorySamples = 0;
 std::uint64_t cpuSpanProbes = 0;
 std::uint64_t metricWrites = 0;
 std::uint64_t gpuTimestampWrites = 0;
 double frameAverageNs = 0.0;
 double frameMinNs = 0.0;
 double frameMaxNs = 0.0;
 double frameP50Ns = 0.0;
 double frameP95Ns = 0.0;
 double frameP99Ns = 0.0;
 double gpuFrameAverageNs = 0.0;
 std::array<double, kRenderMetricCount> metricAverages{};
 std::vector<RenderSpanSnapshot> spans;
};
struct RenderProfileToken {
 int aggregate = -1;
 int gpuQuery = -1;
 std::int64_t cpuStartNs = 0;
};
class RenderProfiler {
 public:
 static RenderProfiler& instance();
 void setEnabled(bool enabled);
 [[nodiscard]] bool enabled() const noexcept {
  return enabled_;
 }
 void beginFrame();
 void endFrame();
 [[nodiscard]] RenderProfileToken beginSpan(std::string_view category,
                                            std::string_view name = {}) noexcept;
 void endSpan(RenderProfileToken token) noexcept;
 void recordSpanDuration(std::string_view category, std::string_view name, std::uint64_t nanos) noexcept;
 void record(RenderMetric metric, std::uint64_t count = 1) noexcept;
 void resetSamples();
 void destroy();
 [[nodiscard]] std::vector<std::string> lines() const;
 [[nodiscard]] RenderProfileSnapshot snapshot() const;
 [[nodiscard]] static const char* metricName(RenderMetric metric);
 [[nodiscard]] static constexpr std::uint32_t gpuQueryInterval() noexcept {
  return kGpuQueryInterval;
 }
 [[nodiscard]] double frameAverageNs() const noexcept {
  return frameAvgNs_;
 }
 [[nodiscard]] double gpuFrameSpanAverageNs() const noexcept {
  return gpuFrameSpanAvgNs_;
 }
 [[nodiscard]] double metricAverage(RenderMetric metric) const noexcept {
  return metricAvg_[static_cast<std::size_t>(metric)];
 }
 [[nodiscard]] std::uint64_t frameMetric(RenderMetric metric) const noexcept {
  return metrics_[static_cast<std::size_t>(metric)];
 }
 [[nodiscard]] double cpuSpanAverageNs(std::string_view name) const noexcept;
 [[nodiscard]] double gpuSpanAverageNs(std::string_view name) const noexcept;

 private:
 static constexpr int kRingDepth = 4;
 static constexpr int kMaxSpans = 64;
 static constexpr int kSpanNameBytes = 72;
 static constexpr std::uint32_t kGpuQueryInterval = 8;
 struct SpanAggregate {
  std::array<char, kSpanNameBytes> name{};
  double cpuAvgNs = 0.0;
  double gpuAvgNs = 0.0;
  std::uint64_t cpuFrameNs = 0;
  std::uint64_t cpuTotalNs = 0;
  std::uint64_t gpuTotalNs = 0;
  std::uint64_t cpuSamples = 0;
  std::uint64_t gpuSamples = 0;
  bool cpuSeeded = false;
  bool gpuSeeded = false;
 };
 [[nodiscard]] int findOrCreateSpan(std::string_view category, std::string_view name) noexcept;
 [[nodiscard]] int findSpan(std::string_view name) const noexcept;
 void ensureQueries();
 [[nodiscard]] bool collectFrameSpan(int slot);
 void releaseQueries() noexcept;
 bool enabled_ = false;
 bool inFrame_ = false;
 bool gpuReady_ = false;
 bool gpuAttempted_ = false;
 bool frameQueryOpen_ = false;
 bool frameSeeded_ = false;
 int ringSlot_ = kRingDepth - 1;
 int aggregateCount_ = 0;
 int gpuSpanCount_ = 0;
 std::uint32_t frameCounter_ = 0;
 std::uint64_t frameSampleCount_ = 0;
 std::uint64_t frameTotalNs_ = 0;
 std::uint64_t gpuFrameSampleCount_ = 0;
 std::uint64_t gpuFrameTotalNs_ = 0;
 std::uint64_t cpuSpanProbes_ = 0;
 std::uint64_t metricWrites_ = 0;
 std::uint64_t gpuTimestampWrites_ = 0;
 std::int64_t frameStartNs_ = 0;
 std::int64_t frameNs_ = 0;
 double frameAvgNs_ = 0.0;
 double gpuFrameSpanAvgNs_ = 0.0;
 std::array<std::uint64_t, kRenderMetricCount> metrics_{};
 std::array<std::uint64_t, kRenderMetricCount> metricTotals_{};
 std::array<double, kRenderMetricCount> metricAvg_{};
 std::array<bool, kRenderMetricCount> metricSeeded_{};
 std::array<SpanAggregate, kMaxSpans> spans_{};
 std::array<std::array<unsigned, 2>, kRingDepth> frameQueries_{};
 std::array<std::array<unsigned, kMaxSpans * 2>, kRingDepth> spanQueries_{};
 std::array<std::array<int, kMaxSpans>, kRingDepth> spanQueryAggregates_{};
 std::array<int, kRingDepth> pendingSpanCounts_{};
 std::array<bool, kRingDepth> frameQueryPending_{};
 static constexpr int kFrameHistory = 4096;
 std::array<double, kFrameHistory> frameHistoryNs_{};
 std::size_t frameHistoryCount_ = 0;
 std::size_t frameHistoryCursor_ = 0;
 double frameMinNs_ = 0.0;
 double frameMaxNs_ = 0.0;
 mutable std::chrono::steady_clock::time_point lastLinesAt_{};
 mutable std::vector<std::string> linesCache_;
};
class RenderProfileScope {
 public:
 explicit RenderProfileScope(std::string_view category) noexcept;
 RenderProfileScope(std::string_view category, std::string_view name) noexcept;
 ~RenderProfileScope();
 RenderProfileScope(const RenderProfileScope&) = delete;
 RenderProfileScope& operator=(const RenderProfileScope&) = delete;

 private:
 RenderProfileToken token_{};
};
} // namespace net::minecraft::client::debug
