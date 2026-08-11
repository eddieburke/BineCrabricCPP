#include "net/minecraft/client/debug/RenderProfiler.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "net/minecraft/client/gl/GLCore.hpp"
namespace net::minecraft::client::debug {
namespace {
constexpr unsigned kTimestamp = 0x8E28;
constexpr unsigned kQueryResult = 0x8866;
constexpr unsigned kQueryResultAvailable = 0x8867;
constexpr double kSmoothing = 0.05;
std::int64_t nanoTime() noexcept {
 return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
     .count();
}
std::string formatMs(double nanos) {
 char buffer[32];
 std::snprintf(buffer, sizeof(buffer), "%.2f", nanos / 1.0e6);
 return buffer;
}
void updateAverage(double sample, double& average, bool& seeded) noexcept {
 if(!seeded) {
  average = sample;
  seeded = true;
 } else {
  average += (sample - average) * kSmoothing;
 }
}
} // namespace
RenderProfiler& RenderProfiler::instance() {
 static RenderProfiler profiler;
 return profiler;
}
const char* RenderProfiler::metricName(RenderMetric metric) {
 switch(metric) {
 case RenderMetric::EntityTraversals: return "Entity traversals";
 case RenderMetric::EntityVisible: return "Entity visible";
 case RenderMetric::EntityCulled: return "Entity culled";
 case RenderMetric::EntityRendererInvocations: return "Renderer invocations";
 case RenderMetric::GeneratedGeometryBatches: return "Generated geometry";
 case RenderMetric::GeneratedVertices: return "Generated vertices";
 case RenderMetric::FilteredGeometryBatches: return "Filtered geometry";
 case RenderMetric::FilteredVertices: return "Filtered vertices";
 case RenderMetric::DrawCalls: return "Draw calls";
 case RenderMetric::DrawVertices: return "Draw vertices";
 case RenderMetric::ProgramBinds: return "Program binds";
 case RenderMetric::TextureBinds: return "Texture binds";
 case RenderMetric::FramebufferBinds: return "Framebuffer binds";
 case RenderMetric::UniformUploads: return "Uniform uploads";
 case RenderMetric::Allocations: return "Allocations";
 case RenderMetric::RawGlQueries: return "Raw GL queries";
 case RenderMetric::AsyncReadbacks: return "Async readbacks";
 case RenderMetric::SynchronousReadbacks: return "Sync readbacks";
 case RenderMetric::Copies: return "Copies";
 case RenderMetric::ArenaGrows: return "Arena grows";
 case RenderMetric::ArenaGrowVertices: return "Arena grow verts";
 case RenderMetric::QuadIndexGrows: return "Quad index grows";
 case RenderMetric::DrawSubBatches: return "Draw sub-batches";
 case RenderMetric::ChunkColumnsPending: return "Chunk columns pending";
 case RenderMetric::ChunkSectionsResident: return "Chunk sections resident";
 case RenderMetric::ChunkSectionsVisible: return "Chunk sections visible";
 case RenderMetric::MeshJobsQueued: return "Mesh jobs queued";
 case RenderMetric::MeshJobsInFlight: return "Mesh jobs in flight";
 case RenderMetric::MeshUploadsPending: return "Mesh uploads pending";
 case RenderMetric::MeshUploadBytes: return "Mesh upload bytes";
 case RenderMetric::ShadowSectionsVisible: return "Shadow sections visible";
 case RenderMetric::ShaderPasses: return "Shader passes";
 case RenderMetric::ComputeDispatches: return "Compute dispatches";
 case RenderMetric::ComputeWorkgroups: return "Compute workgroups";
 case RenderMetric::MemoryBarriers: return "Memory barriers";
 case RenderMetric::MipmapGenerations: return "Mipmap generations";
 case RenderMetric::ShadowDepthCopies: return "Shadow depth copies";
 case RenderMetric::RenderTargetAllocations: return "Render target allocations";
 default: return "?";
 }
}
void RenderProfiler::setEnabled(bool enabled) {
 if(enabled_ == enabled) {
  return;
 }
 enabled_ = enabled;
 if(!enabled_) {
  destroy();
 }
}
int RenderProfiler::findOrCreateSpan(std::string_view category, std::string_view name) noexcept {
 std::array<char, kSpanNameBytes> combined{};
 std::size_t length = 0;
 auto append = [&](std::string_view value) {
  for(char ch : value) {
   if(length + 1 >= combined.size()) {
    break;
   }
   combined[length++] = ch;
  }
 };
 append(category);
 if(!category.empty() && !name.empty() && length + 1 < combined.size()) {
  combined[length++] = '/';
 }
 append(name);
 combined[length] = '\0';
 const std::string_view target(combined.data(), length);
 for(int i = 0; i < aggregateCount_; ++i) {
  if(std::string_view(spans_[static_cast<std::size_t>(i)].name.data()) == target) {
   return i;
  }
 }
 if(target.empty() || aggregateCount_ >= kMaxSpans) {
  return -1;
 }
 const int index = aggregateCount_++;
 spans_[static_cast<std::size_t>(index)].name = combined;
 return index;
}
int RenderProfiler::findSpan(std::string_view name) const noexcept {
 for(int i = 0; i < aggregateCount_; ++i) {
  if(std::string_view(spans_[static_cast<std::size_t>(i)].name.data()) == name) {
   return i;
  }
 }
 return -1;
}
void RenderProfiler::releaseQueries() noexcept {
 try {
  if(gl::GLCore::deleteQueries != nullptr) {
   for(int slot = 0; slot < kRingDepth; ++slot) {
    auto& frame = frameQueries_[static_cast<std::size_t>(slot)];
    gl::GLCore::deleteQueries(static_cast<int>(frame.size()), frame.data());
    frame.fill(0);
    auto& spans = spanQueries_[static_cast<std::size_t>(slot)];
    gl::GLCore::deleteQueries(static_cast<int>(spans.size()), spans.data());
    spans.fill(0);
   }
  }
 } catch(...) {
 }
}
void RenderProfiler::ensureQueries() {
 if(gpuReady_ || gpuAttempted_) {
  return;
 }
 gpuAttempted_ = true;
 try {
  gl::GLCore::ensureLoaded();
  if(!gl::GLCore::timerQuerySupported || gl::GLCore::queryCounter == nullptr ||
     gl::GLCore::genQueries == nullptr || gl::GLCore::getQueryObjectiv == nullptr ||
     gl::GLCore::getQueryObjectui64v == nullptr) {
   return;
  }
  for(int slot = 0; slot < kRingDepth; ++slot) {
   auto& frame = frameQueries_[static_cast<std::size_t>(slot)];
   gl::GLCore::genQueries(static_cast<int>(frame.size()), frame.data());
   auto& spans = spanQueries_[static_cast<std::size_t>(slot)];
   gl::GLCore::genQueries(static_cast<int>(spans.size()), spans.data());
   if(std::find(frame.begin(), frame.end(), 0U) != frame.end() ||
      std::find(spans.begin(), spans.end(), 0U) != spans.end()) {
    releaseQueries();
    return;
   }
  }
  gpuReady_ = true;
 } catch(...) {
  releaseQueries();
  gpuReady_ = false;
 }
}
bool RenderProfiler::collectFrameSpan(int slot) {
 const auto index = static_cast<std::size_t>(slot);
 if(!gpuReady_ || !frameQueryPending_[index]) {
  return !frameQueryPending_[index];
 }
 try {
  int available = 0;
  gl::GLCore::getQueryObjectiv(frameQueries_[index][1], kQueryResultAvailable, &available);
  if(available == 0) {
   return false;
  }
  std::uint64_t frameStart = 0;
  std::uint64_t frameEnd = 0;
  gl::GLCore::getQueryObjectui64v(frameQueries_[index][0], kQueryResult, &frameStart);
  gl::GLCore::getQueryObjectui64v(frameQueries_[index][1], kQueryResult, &frameEnd);
  if(frameEnd > frameStart) {
   gpuFrameTotalNs_ += frameEnd - frameStart;
   ++gpuFrameSampleCount_;
   bool seeded = gpuFrameSpanAvgNs_ > 0.0;
   updateAverage(static_cast<double>(frameEnd - frameStart), gpuFrameSpanAvgNs_, seeded);
  }
  const int count = pendingSpanCounts_[index];
  for(int query = 0; query < count; ++query) {
   const int aggregate = spanQueryAggregates_[index][static_cast<std::size_t>(query)];
   if(aggregate < 0 || aggregate >= aggregateCount_) {
    continue;
   }
   std::uint64_t begin = 0;
   std::uint64_t end = 0;
   gl::GLCore::getQueryObjectui64v(spanQueries_[index][static_cast<std::size_t>(query * 2)], kQueryResult, &begin);
   gl::GLCore::getQueryObjectui64v(spanQueries_[index][static_cast<std::size_t>(query * 2 + 1)], kQueryResult, &end);
   if(end > begin) {
    SpanAggregate& span = spans_[static_cast<std::size_t>(aggregate)];
    span.gpuTotalNs += end - begin;
    ++span.gpuSamples;
    updateAverage(static_cast<double>(end - begin), span.gpuAvgNs, span.gpuSeeded);
   }
  }
  pendingSpanCounts_[index] = 0;
  frameQueryPending_[index] = false;
  return true;
 } catch(...) {
  gpuReady_ = false;
  return false;
 }
}
void RenderProfiler::beginFrame() {
 if(!enabled_) {
  return;
 }
 ensureQueries();
 ++frameCounter_;
 frameQueryOpen_ = false;
 gpuSpanCount_ = 0;
 if(frameCounter_ % kGpuQueryInterval == 0) {
  const int next = (ringSlot_ + 1) % kRingDepth;
  if(collectFrameSpan(next)) {
   ringSlot_ = next;
   try {
    if(gpuReady_) {
     gl::GLCore::queryCounter(frameQueries_[static_cast<std::size_t>(ringSlot_)][0], kTimestamp);
     ++gpuTimestampWrites_;
     frameQueryOpen_ = true;
    }
   } catch(...) {
    gpuReady_ = false;
   }
  }
 }
 metrics_.fill(0);
 for(int i = 0; i < aggregateCount_; ++i) {
  spans_[static_cast<std::size_t>(i)].cpuFrameNs = 0;
 }
 frameStartNs_ = nanoTime();
 inFrame_ = true;
}
RenderProfileToken RenderProfiler::beginSpan(std::string_view category, std::string_view name) noexcept {
 RenderProfileToken token;
 if(!inFrame_) {
  return token;
 }
 ++cpuSpanProbes_;
 token.aggregate = findOrCreateSpan(category, name);
 if(token.aggregate < 0) {
  return token;
 }
 token.cpuStartNs = nanoTime();
 if(frameQueryOpen_ && gpuSpanCount_ < kMaxSpans) {
  const int query = gpuSpanCount_++;
  token.gpuQuery = query;
  spanQueryAggregates_[static_cast<std::size_t>(ringSlot_)][static_cast<std::size_t>(query)] = token.aggregate;
  try {
   gl::GLCore::queryCounter(
       spanQueries_[static_cast<std::size_t>(ringSlot_)][static_cast<std::size_t>(query * 2)], kTimestamp);
   ++gpuTimestampWrites_;
  } catch(...) {
   token.gpuQuery = -1;
   gpuReady_ = false;
   frameQueryOpen_ = false;
  }
 }
 return token;
}
void RenderProfiler::endSpan(RenderProfileToken token) noexcept {
 if(token.aggregate < 0 || token.aggregate >= aggregateCount_ || token.cpuStartNs == 0) {
  return;
 }
 const std::int64_t elapsed = nanoTime() - token.cpuStartNs;
 if(elapsed > 0) {
  spans_[static_cast<std::size_t>(token.aggregate)].cpuFrameNs += static_cast<std::uint64_t>(elapsed);
 }
 if(token.gpuQuery >= 0 && gpuReady_) {
  try {
   gl::GLCore::queryCounter(
       spanQueries_[static_cast<std::size_t>(ringSlot_)][static_cast<std::size_t>(token.gpuQuery * 2 + 1)],
       kTimestamp);
   ++gpuTimestampWrites_;
  } catch(...) {
   gpuReady_ = false;
   frameQueryOpen_ = false;
  }
 }
}
void RenderProfiler::recordSpanDuration(std::string_view category,
                                        std::string_view name,
                                        std::uint64_t nanos) noexcept {
 if(!inFrame_ || nanos == 0) {
  return;
 }
 const int aggregate = findOrCreateSpan(category, name);
 if(aggregate >= 0) {
  spans_[static_cast<std::size_t>(aggregate)].cpuFrameNs += nanos;
 }
}
void RenderProfiler::endFrame() {
 if(!inFrame_) {
  return;
 }
 inFrame_ = false;
 if(frameQueryOpen_) {
  const auto slot = static_cast<std::size_t>(ringSlot_);
  try {
   gl::GLCore::queryCounter(frameQueries_[slot][1], kTimestamp);
   ++gpuTimestampWrites_;
   pendingSpanCounts_[slot] = gpuSpanCount_;
   frameQueryPending_[slot] = true;
  } catch(...) {
   gpuReady_ = false;
  }
  frameQueryOpen_ = false;
 }
 frameNs_ = nanoTime() - frameStartNs_;
 ++frameSampleCount_;
 frameTotalNs_ += static_cast<std::uint64_t>(std::max<std::int64_t>(0, frameNs_));
 const double frameSample = static_cast<double>(frameNs_);
 if(frameSampleCount_ == 1) {
  frameMinNs_ = frameSample;
  frameMaxNs_ = frameSample;
 } else {
  frameMinNs_ = std::min(frameMinNs_, frameSample);
  frameMaxNs_ = std::max(frameMaxNs_, frameSample);
 }
 frameHistoryNs_[frameHistoryCursor_] = frameSample;
 frameHistoryCursor_ = (frameHistoryCursor_ + 1) % frameHistoryNs_.size();
 frameHistoryCount_ = std::min(frameHistoryCount_ + 1, frameHistoryNs_.size());
 updateAverage(static_cast<double>(frameNs_), frameAvgNs_, frameSeeded_);
 for(int metric = 0; metric < kRenderMetricCount; ++metric) {
  const auto index = static_cast<std::size_t>(metric);
  if(metrics_[index] != 0 || metricSeeded_[index]) {
   updateAverage(static_cast<double>(metrics_[index]), metricAvg_[index], metricSeeded_[index]);
  }
  metricTotals_[index] += metrics_[index];
 }
 for(int i = 0; i < aggregateCount_; ++i) {
  SpanAggregate& span = spans_[static_cast<std::size_t>(i)];
  if(span.cpuFrameNs != 0 || span.cpuSeeded) {
   updateAverage(static_cast<double>(span.cpuFrameNs), span.cpuAvgNs, span.cpuSeeded);
  }
  span.cpuTotalNs += span.cpuFrameNs;
  ++span.cpuSamples;
 }
}
void RenderProfiler::record(RenderMetric metric, std::uint64_t count) noexcept {
 if(inFrame_) {
  ++metricWrites_;
  metrics_[static_cast<std::size_t>(metric)] += count;
 }
}
double RenderProfiler::cpuSpanAverageNs(std::string_view name) const noexcept {
 const int index = findSpan(name);
 return index < 0 ? 0.0 : spans_[static_cast<std::size_t>(index)].cpuAvgNs;
}
double RenderProfiler::gpuSpanAverageNs(std::string_view name) const noexcept {
 const int index = findSpan(name);
 return index < 0 ? 0.0 : spans_[static_cast<std::size_t>(index)].gpuAvgNs;
}
void RenderProfiler::resetSamples() {
 destroy();
}
void RenderProfiler::destroy() {
 releaseQueries();
 frameQueryPending_.fill(false);
 pendingSpanCounts_.fill(0);
 frameQueryOpen_ = false;
 gpuReady_ = false;
 gpuAttempted_ = false;
 frameSeeded_ = false;
 frameCounter_ = 0;
 frameSampleCount_ = 0;
 frameTotalNs_ = 0;
 gpuFrameSampleCount_ = 0;
 gpuFrameTotalNs_ = 0;
 cpuSpanProbes_ = 0;
 metricWrites_ = 0;
 gpuTimestampWrites_ = 0;
 ringSlot_ = kRingDepth - 1;
 aggregateCount_ = 0;
 gpuSpanCount_ = 0;
 inFrame_ = false;
 metrics_.fill(0);
 metricTotals_.fill(0);
 metricAvg_.fill(0.0);
 metricSeeded_.fill(false);
 spans_.fill({});
 frameAvgNs_ = 0.0;
 gpuFrameSpanAvgNs_ = 0.0;
 frameHistoryNs_.fill(0.0);
 frameHistoryCount_ = 0;
 frameHistoryCursor_ = 0;
 frameMinNs_ = 0.0;
 frameMaxNs_ = 0.0;
 linesCache_.clear();
 lastLinesAt_ = {};
}
RenderProfileSnapshot RenderProfiler::snapshot() const {
 RenderProfileSnapshot result;
 result.frames = frameSampleCount_;
 result.gpuFrames = gpuFrameSampleCount_;
 result.frameHistorySamples = frameHistoryCount_;
 result.cpuSpanProbes = cpuSpanProbes_;
 result.metricWrites = metricWrites_;
 result.gpuTimestampWrites = gpuTimestampWrites_;
 result.frameAverageNs = frameSampleCount_ == 0 ? 0.0 : static_cast<double>(frameTotalNs_) / frameSampleCount_;
 result.frameMinNs = frameMinNs_;
 result.frameMaxNs = frameMaxNs_;
 result.gpuFrameAverageNs = gpuFrameSampleCount_ == 0
                                ? 0.0
                                : static_cast<double>(gpuFrameTotalNs_) / gpuFrameSampleCount_;
 for(int metric = 0; metric < kRenderMetricCount; ++metric) {
  result.metricAverages[static_cast<std::size_t>(metric)] = frameSampleCount_ == 0
                                                                ? 0.0
                                                                : static_cast<double>(metricTotals_[static_cast<std::size_t>(metric)]) / frameSampleCount_;
 }
 std::vector<double> history(frameHistoryNs_.begin(), frameHistoryNs_.begin() +
                                                          static_cast<std::ptrdiff_t>(frameHistoryCount_));
 std::sort(history.begin(), history.end());
 const auto percentile = [&](double value) {
  if(history.empty()) {
   return 0.0;
  }
  const std::size_t index = static_cast<std::size_t>(
      std::clamp(value * static_cast<double>(history.size() - 1), 0.0,
                 static_cast<double>(history.size() - 1)));
  return history[index];
 };
 result.frameP50Ns = percentile(0.50);
 result.frameP95Ns = percentile(0.95);
 result.frameP99Ns = percentile(0.99);
 result.spans.reserve(static_cast<std::size_t>(aggregateCount_));
 for(int i = 0; i < aggregateCount_; ++i) {
  const SpanAggregate& span = spans_[static_cast<std::size_t>(i)];
  result.spans.push_back({span.name.data(),
                          span.cpuSamples == 0 ? 0.0 : static_cast<double>(span.cpuTotalNs) / span.cpuSamples,
                          span.gpuSamples == 0 ? 0.0 : static_cast<double>(span.gpuTotalNs) / span.gpuSamples,
                          span.cpuSamples,
                          span.gpuSamples});
 }
 std::sort(result.spans.begin(), result.spans.end(), [](const RenderSpanSnapshot& a, const RenderSpanSnapshot& b) {
  return std::max(a.cpuAverageNs, a.gpuAverageNs) > std::max(b.cpuAverageNs, b.gpuAverageNs);
 });
 return result;
}
std::vector<std::string> RenderProfiler::lines() const {
 if(!enabled_) {
  return {};
 }
 const auto now = std::chrono::steady_clock::now();
 if(!linesCache_.empty() && now - lastLinesAt_ < std::chrono::milliseconds(250)) {
  return linesCache_;
 }
 std::vector<std::string> out;
 out.reserve(static_cast<std::size_t>(kRenderMetricCount + 20));
 out.push_back("Frame: " + formatMs(frameAvgNs_) + " ms");
 if(gpuReady_ && gpuFrameSpanAvgNs_ > 0.0) {
  out.push_back("GPU frame span: " + formatMs(gpuFrameSpanAvgNs_) + " ms");
 }
 std::array<int, kMaxSpans> order{};
 for(int i = 0; i < aggregateCount_; ++i) {
  order[static_cast<std::size_t>(i)] = i;
 }
 std::sort(order.begin(), order.begin() + aggregateCount_, [this](int a, int b) {
  const SpanAggregate& left = spans_[static_cast<std::size_t>(a)];
  const SpanAggregate& right = spans_[static_cast<std::size_t>(b)];
  return std::max(left.cpuAvgNs, left.gpuAvgNs) > std::max(right.cpuAvgNs, right.gpuAvgNs);
 });
 constexpr int kDisplayedSpans = 14;
 for(int rank = 0; rank < std::min(aggregateCount_, kDisplayedSpans); ++rank) {
  const SpanAggregate& span = spans_[static_cast<std::size_t>(order[static_cast<std::size_t>(rank)])];
  if(span.cpuAvgNs <= 0.0 && span.gpuAvgNs <= 0.0) {
   continue;
  }
  std::string line(span.name.data());
  line += ": CPU " + formatMs(span.cpuAvgNs);
  if(span.gpuAvgNs > 0.0) {
   line += " / GPU " + formatMs(span.gpuAvgNs);
  }
  line += " ms";
  out.push_back(std::move(line));
 }
 for(int metric = 0; metric < kRenderMetricCount; ++metric) {
  const auto index = static_cast<std::size_t>(metric);
  if(metricAvg_[index] <= 0.0) {
   continue;
  }
  out.push_back(std::string(metricName(static_cast<RenderMetric>(metric))) + ": " +
                std::to_string(static_cast<std::uint64_t>(metricAvg_[index] + 0.5)));
 }
 linesCache_ = out;
 lastLinesAt_ = now;
 return out;
}
RenderProfileScope::RenderProfileScope(std::string_view category) noexcept
    : token_(RenderProfiler::instance().beginSpan(category)) {
}
RenderProfileScope::RenderProfileScope(std::string_view category, std::string_view name) noexcept
    : token_(RenderProfiler::instance().beginSpan(category, name)) {
}
RenderProfileScope::~RenderProfileScope() {
 RenderProfiler::instance().endSpan(token_);
}
} // namespace net::minecraft::client::debug
