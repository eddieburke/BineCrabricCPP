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
} // namespace
RenderProfiler& RenderProfiler::instance() {
 static RenderProfiler profiler;
 return profiler;
}
const char* RenderProfiler::stageName(RenderStage stage) {
 switch(stage) {
 case RenderStage::Sky:
  return "sky";
 case RenderStage::Cull:
  return "cull";
 case RenderStage::Compile:
  return "chunks";
 case RenderStage::SolidTerrain:
  return "solid";
 case RenderStage::Entities:
  return "entities";
 case RenderStage::Particles:
  return "particles";
 case RenderStage::TranslucentTerrain:
  return "translucent";
 case RenderStage::Clouds:
  return "clouds";
 case RenderStage::Hand:
  return "hand";
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
void RenderProfiler::collectQueries() {
 if(!gpuReady_) {
  return;
 }
 try {
  const int slot = (ringSlot_ + 1) % kRingDepth;
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
  collectQueries();
 }
 ringSlot_ = (ringSlot_ + 1) % kRingDepth;
 cpuNs_.fill(0);
 activeStage_ = -1;
 frameStartNs_ = nanoTime();
 inFrame_ = true;
}
void RenderProfiler::endFrame() {
 if(!inFrame_) {
  return;
 }
 if(activeStage_ >= 0) {
  endStage(static_cast<RenderStage>(activeStage_));
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
 if(!inFrame_ || activeStage_ >= 0) {
  return;
 }
 activeStage_ = static_cast<int>(stage);
 activeQueryBegun_ = false;
 stageStartNs_ = nanoTime();
 if(gpuReady_ && gpuActiveThisFrame_) {
  const auto slot = static_cast<std::size_t>(ringSlot_);
  const auto index = static_cast<std::size_t>(activeStage_);
  if(!queryPending_[slot][index]) {
   gl::GLCore::beginQuery(kTimeElapsed, queries_[slot][index]);
   queryPending_[slot][index] = true;
   activeQueryBegun_ = true;
  }
 }
}
void RenderProfiler::endStage(RenderStage stage) {
 if(!inFrame_ || activeStage_ != static_cast<int>(stage)) {
  return;
 }
 const auto index = static_cast<std::size_t>(activeStage_);
 cpuNs_[index] += nanoTime() - stageStartNs_;
 if(gpuReady_ && activeQueryBegun_) {
  gl::GLCore::endQuery(kTimeElapsed);
 }
 activeQueryBegun_ = false;
 activeStage_ = -1;
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
 inFrame_ = false;
 activeStage_ = -1;
 cpuAvgNs_.fill(0.0);
 gpuAvgNs_.fill(0.0);
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
 out.reserve(static_cast<std::size_t>(kRenderStageCount) + 3);
 out.push_back(gpuReady_ ? std::string("stage      cpu    gpu") : std::string("stage      cpu"));
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
  std::string line = stageName(static_cast<RenderStage>(stage));
  line.resize(12, ' ');
  line += formatMs(cpuAvgNs_[index]);
  if(gpuReady_) {
   line.resize(19, ' ');
   line += formatMs(gpuAvgNs_[index]);
  }
  out.push_back(line);
 }
 std::string total = "measured";
 total.resize(12, ' ');
 total += formatMs(cpuAccounted);
 if(gpuReady_) {
  total.resize(19, ' ');
  total += formatMs(gpuMeasuredAvgNs_);
 }
 out.push_back(total);
  std::string other = "unmeasured";
  other.resize(12, ' ');
  other += formatMs(std::max(0.0, frameAvgNs_ - cpuAccounted));
  out.push_back(other);
  linesCache_ = out;
  lastLinesAt_ = now;
  return out;
}
} // namespace net::minecraft::client::debug
