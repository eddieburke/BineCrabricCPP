#include "net/minecraft/client/debug/RenderProfiler.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include "net/minecraft/client/gl/GLCore.hpp"
namespace net::minecraft::client::debug {
namespace {
constexpr unsigned kTimeElapsed = 0x88BF;
constexpr unsigned kQueryResult = 0x8866;
constexpr unsigned kQueryResultAvailable = 0x8867;
constexpr double kSmoothing = 0.05;
std::int64_t nanoTime() {
 return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
     .count();
}
std::string formatMs(double nanos) {
 char buffer[32];
 std::snprintf(buffer, sizeof(buffer), "%.2f", nanos / 1.0e6);
 return buffer;
}
std::string paddedColumn(std::string text, std::size_t width) {
 if(text.size() < width) {
  text.append(width - text.size(), ' ');
 }
 return text;
}
constexpr std::size_t kStageColumnWidth = 18;
constexpr std::size_t kCpuColumnWidth = 7;
} // namespace
RenderProfiler& RenderProfiler::instance() {
 static RenderProfiler profiler;
 return profiler;
}
const char* RenderProfiler::stageName(RenderStage stage) {
 switch(stage) {
 case RenderStage::FrameSetup:
  return "Frame setup";
 case RenderStage::FrameDrain:
  return "Frame drain";
 case RenderStage::Input:
  return "Input";
 case RenderStage::Ticks:
  return "Ticks";
 case RenderStage::RenderOverhead:
  return "Render overhead";
 case RenderStage::PackPrepare:
  return "Pack prepare";
 case RenderStage::Shadow:
  return "Shadow";
 case RenderStage::ShadowComposite:
  return "Shadow composite";
 case RenderStage::Sky:
  return "Sky";
 case RenderStage::Cull:
  return "Frustum cull";
 case RenderStage::Compile:
  return "Chunk meshes";
 case RenderStage::SolidTerrain:
  return "Solid terrain";
 case RenderStage::Entities:
  return "Entities";
 case RenderStage::Particles:
  return "Particles";
 case RenderStage::OpaqueDepth:
  return "Opaque depth";
 case RenderStage::TranslucentTerrain:
  return "Translucent terrain";
 case RenderStage::Clouds:
  return "Clouds";
 case RenderStage::Hand:
  return "Hand";
 case RenderStage::HandDepth:
  return "Hand depth";
 case RenderStage::Deferred:
  return "Deferred";
 case RenderStage::CenterDepth:
  return "Center depth";
 case RenderStage::PostProcess:
  return "Post process";
 case RenderStage::HudGui:
  return "HUD / GUI";
 case RenderStage::Present:
  return "Present";
 case RenderStage::Pace:
  return "Pace";
 case RenderStage::Diagnostics:
  return "Diagnostics";
 case RenderStage::EntityCollectCull:
  return "Entity collect/cull";
 case RenderStage::EntityPreparation:
  return "Entity preparation";
 case RenderStage::EntityOpaque:
  return "Entity opaque";
 case RenderStage::EntityTranslucent:
  return "Entity translucent";
 case RenderStage::BlockEntities:
  return "Block entities";
 case RenderStage::EntityDebugLabels:
  return "Entity debug labels";
 case RenderStage::ShadowEntityPreparation:
  return "Shadow entity prep";
 case RenderStage::ShadowEntityDraw:
  return "Shadow entity draw";
 case RenderStage::TargetClear:
  return "Target clear";
 case RenderStage::DepthCaptureCopy:
  return "Depth capture/copy";
 case RenderStage::CenterDepthSampling:
  return "Center-depth sampling";
 case RenderStage::DeferredScheduling:
  return "Deferred scheduling";
 case RenderStage::FramebufferTransitions:
  return "Framebuffer transitions";
 case RenderStage::ModMeshes:
  return "Mod meshes";
 case RenderStage::RenderOrchestration:
  return "Render orchestration";
 default:
  return "?";
 }
}
const char* RenderProfiler::metricName(RenderMetric metric) {
 switch(metric) {
 case RenderMetric::EntityTraversals:
  return "Entity traversals";
 case RenderMetric::EntityVisible:
  return "Entity visible";
 case RenderMetric::EntityCulled:
  return "Entity culled";
 case RenderMetric::EntityRendererInvocations:
  return "Renderer invocations";
 case RenderMetric::GeneratedGeometryBatches:
  return "Generated geometry";
 case RenderMetric::GeneratedVertices:
  return "Generated vertices";
 case RenderMetric::FilteredGeometryBatches:
  return "Filtered geometry";
 case RenderMetric::FilteredVertices:
  return "Filtered vertices";
 case RenderMetric::DrawCalls:
  return "Draw calls";
 case RenderMetric::DrawVertices:
  return "Draw vertices";
 case RenderMetric::ProgramBinds:
  return "Program binds";
 case RenderMetric::TextureBinds:
  return "Texture binds";
 case RenderMetric::FramebufferBinds:
  return "Framebuffer binds";
 case RenderMetric::UniformUploads:
  return "Uniform uploads";
 case RenderMetric::Allocations:
  return "Allocations";
 case RenderMetric::RawGlQueries:
  return "Raw GL queries";
 case RenderMetric::AsyncReadbacks:
  return "Async readbacks";
 case RenderMetric::SynchronousReadbacks:
  return "Sync readbacks";
 case RenderMetric::Copies:
  return "Copies";
 default:
  return "?";
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
void RenderProfiler::ensureQueries() {
 if(gpuReady_ || gpuAttempted_) {
  return;
 }
 gpuAttempted_ = true;
 try {
  gl::GLCore::ensureLoaded();
  if(!gl::GLCore::timerQuerySupported) {
   return;
  }
  if(gl::GLCore::genQueries == nullptr || gl::GLCore::deleteQueries == nullptr ||
     gl::GLCore::beginQuery == nullptr || gl::GLCore::endQuery == nullptr ||
     gl::GLCore::getQueryObjectiv == nullptr || gl::GLCore::getQueryObjectui64v == nullptr) {
   return;
  }
  for(int slot = 0; slot < kRingDepth; ++slot) {
   gl::GLCore::genQueries(kRenderStageCount, queries_[static_cast<std::size_t>(slot)].data());
   for(int stage = 0; stage < kRenderStageCount; ++stage) {
    if(queries_[static_cast<std::size_t>(slot)][static_cast<std::size_t>(stage)] == 0) {
     return;
    }
   }
  }
  gpuReady_ = true;
 } catch(...) {
  gpuReady_ = false;
 }
}
void RenderProfiler::collectQueries(int slot) {
 if(!gpuReady_) {
  return;
 }
 try {
  double gpuMeasured = 0.0;
  for(int stage = 0; stage < kRenderStageCount; ++stage) {
   const auto index = static_cast<std::size_t>(stage);
   if(!queryPending_[static_cast<std::size_t>(slot)][index]) {
    gpuMeasured += gpuAvgNs_[index];
    continue;
   }
   const unsigned query = queries_[static_cast<std::size_t>(slot)][index];
   int available = 0;
   gl::GLCore::getQueryObjectiv(query, kQueryResultAvailable, &available);
   if(available == 0) {
    gpuMeasured += gpuAvgNs_[index];
    continue;
   }
   std::uint64_t elapsed = 0;
   gl::GLCore::getQueryObjectui64v(query, kQueryResult, &elapsed);
   queryPending_[static_cast<std::size_t>(slot)][index] = false;
   gpuAvgNs_[index] += (static_cast<double>(elapsed) - gpuAvgNs_[index]) * kSmoothing;
   gpuMeasured += gpuAvgNs_[index];
  }
  gpuMeasuredAvgNs_ = gpuMeasured;
 } catch(...) {
  gpuReady_ = false;
  gpuActiveThisFrame_ = false;
 }
}
void RenderProfiler::beginFrame() {
 if(!enabled_) {
  return;
 }
 ensureQueries();
 ++frameCounter_;
 gpuActiveThisFrame_ = (frameCounter_ % kGpuQueryInterval == 0);
 if(gpuActiveThisFrame_) {
  ringSlot_ = (ringSlot_ + 1) % kRingDepth;
  collectQueries(ringSlot_);
 }
 cpuNs_.fill(0);
 metrics_.fill(0);
 activeStageDepth_ = 0;
 frameStartNs_ = nanoTime();
 inFrame_ = true;
}
void RenderProfiler::endFrame() {
 if(!inFrame_) {
  return;
 }
 while(activeStageDepth_ > 0) {
  endStage(static_cast<RenderStage>(activeStages_[static_cast<std::size_t>(activeStageDepth_ - 1)].stage));
 }
 inFrame_ = false;
 frameNs_ = nanoTime() - frameStartNs_;
 frameAvgNs_ += (static_cast<double>(frameNs_) - frameAvgNs_) * kSmoothing;
 for(int stage = 0; stage < kRenderStageCount; ++stage) {
  const auto index = static_cast<std::size_t>(stage);
  cpuAvgNs_[index] += (static_cast<double>(cpuNs_[index]) - cpuAvgNs_[index]) * kSmoothing;
 }
}
void RenderProfiler::beginStage(RenderStage stage) {
 if(!inFrame_ || activeStageDepth_ >= kMaxStageDepth) {
  return;
 }
 ActiveStage& active = activeStages_[static_cast<std::size_t>(activeStageDepth_)];
 active.stage = static_cast<int>(stage);
 active.startNs = nanoTime();
 active.childNs = 0;
 active.queryBegun = false;
 const bool cpuOnly = stage == RenderStage::FrameDrain || stage == RenderStage::Input ||
                      stage == RenderStage::Ticks || stage == RenderStage::RenderOverhead ||
                      stage == RenderStage::Present || stage == RenderStage::Pace ||
                      stage == RenderStage::Diagnostics;
 bool ancestorQuery = false;
 for(int depth = 0; depth < activeStageDepth_; ++depth) {
  ancestorQuery = ancestorQuery || activeStages_[static_cast<std::size_t>(depth)].queryBegun;
 }
 if(gpuReady_ && gpuActiveThisFrame_ && !cpuOnly && !ancestorQuery) {
  const auto slot = static_cast<std::size_t>(ringSlot_);
  const auto index = static_cast<std::size_t>(active.stage);
  if(!queryPending_[slot][index]) {
   gl::GLCore::beginQuery(kTimeElapsed, queries_[slot][index]);
   queryPending_[slot][index] = true;
   active.queryBegun = true;
  }
 }
 ++activeStageDepth_;
}
void RenderProfiler::endStage(RenderStage stage) {
 if(!inFrame_ || activeStageDepth_ <= 0) {
  return;
 }
 ActiveStage& active = activeStages_[static_cast<std::size_t>(activeStageDepth_ - 1)];
 if(active.stage != static_cast<int>(stage)) {
  return;
 }
 const std::int64_t elapsed = nanoTime() - active.startNs;
 const auto index = static_cast<std::size_t>(active.stage);
 cpuNs_[index] += std::max<std::int64_t>(0, elapsed - active.childNs);
 if(gpuReady_ && active.queryBegun) {
  gl::GLCore::endQuery(kTimeElapsed);
 }
 --activeStageDepth_;
 if(activeStageDepth_ > 0) {
  activeStages_[static_cast<std::size_t>(activeStageDepth_ - 1)].childNs += elapsed;
 }
 for(int metric = 0; metric < kRenderMetricCount; ++metric) {
  const auto index = static_cast<std::size_t>(metric);
  metricAvg_[index] += (static_cast<double>(metrics_[index]) - metricAvg_[index]) * kSmoothing;
 }
}
void RenderProfiler::record(RenderMetric metric, std::uint64_t count) noexcept {
 if(!inFrame_) {
  return;
 }
 metrics_[static_cast<std::size_t>(metric)] += count;
}
void RenderProfiler::destroy() {
 if(gpuReady_) {
  for(int slot = 0; slot < kRingDepth; ++slot) {
   gl::GLCore::deleteQueries(kRenderStageCount, queries_[static_cast<std::size_t>(slot)].data());
   queries_[static_cast<std::size_t>(slot)].fill(0);
   queryPending_[static_cast<std::size_t>(slot)].fill(false);
  }
 }
 gpuReady_ = false;
 gpuAttempted_ = false;
 gpuActiveThisFrame_ = false;
 frameCounter_ = 0;
 ringSlot_ = kRingDepth - 1;
 inFrame_ = false;
 activeStageDepth_ = 0;
 cpuAvgNs_.fill(0.0);
 gpuAvgNs_.fill(0.0);
 metrics_.fill(0);
 metricAvg_.fill(0.0);
 frameAvgNs_ = 0.0;
 gpuMeasuredAvgNs_ = 0.0;
 linesCache_.clear();
 lastLinesAt_ = {};
}
std::vector<std::string> RenderProfiler::lines() const {
 std::vector<std::string> out;
 if(!enabled_) {
  return out;
 }
 const auto now = std::chrono::steady_clock::now();
 if(!linesCache_.empty() && now - lastLinesAt_ < std::chrono::milliseconds(250)) {
  return linesCache_;
 }
 out.reserve(static_cast<std::size_t>(kRenderStageCount + kRenderMetricCount) + 3);
 std::string header = paddedColumn("Stage", kStageColumnWidth) + paddedColumn("CPU", kCpuColumnWidth);
 if(gpuReady_) {
  header += "GPU";
 }
 out.push_back(header);
 double cpuAccounted = 0.0;
 std::array<int, kRenderStageCount> order{};
 for(int stage = 0; stage < kRenderStageCount; ++stage) {
  order[static_cast<std::size_t>(stage)] = stage;
  cpuAccounted += cpuAvgNs_[static_cast<std::size_t>(stage)];
 }
 std::sort(order.begin(), order.end(), [this](int a, int b) {
  return cpuAvgNs_[static_cast<std::size_t>(a)] + gpuAvgNs_[static_cast<std::size_t>(a)] >
         cpuAvgNs_[static_cast<std::size_t>(b)] + gpuAvgNs_[static_cast<std::size_t>(b)];
 });
 for(const int stage : order) {
  const auto index = static_cast<std::size_t>(stage);
  std::string line = paddedColumn(stageName(static_cast<RenderStage>(stage)), kStageColumnWidth);
  line += paddedColumn(formatMs(cpuAvgNs_[index]), kCpuColumnWidth);
  if(gpuReady_) {
   line += formatMs(gpuAvgNs_[index]);
  }
  out.push_back(line);
 }
 std::string total = paddedColumn("Total measured", kStageColumnWidth);
 total += paddedColumn(formatMs(cpuAccounted), kCpuColumnWidth);
 if(gpuReady_) {
  total += formatMs(gpuMeasuredAvgNs_);
 }
 out.push_back(total);
 std::string other = paddedColumn("Unmeasured", kStageColumnWidth);
 other += paddedColumn(formatMs(std::max(0.0, frameAvgNs_ - cpuAccounted)), kCpuColumnWidth);
 out.push_back(other);
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
} // namespace net::minecraft::client::debug
