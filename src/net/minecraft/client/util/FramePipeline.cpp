#include "net/minecraft/client/util/FramePipeline.hpp"
#include "net/minecraft/client/debug/RenderProfiler.hpp"
#include "net/minecraft/util/concurrent/FrameBudget.hpp"
namespace net::minecraft::client::util {
namespace {
debug::RenderStage profilerStage(FramePipeline::Phase phase) {
 switch(phase) {
 case FramePipeline::Phase::Drain:
  return debug::RenderStage::FrameDrain;
 case FramePipeline::Phase::Input:
  return debug::RenderStage::Input;
 case FramePipeline::Phase::Ticks:
  return debug::RenderStage::Ticks;
 case FramePipeline::Phase::Render:
  return debug::RenderStage::RenderOverhead;
 case FramePipeline::Phase::Present:
  return debug::RenderStage::Present;
 case FramePipeline::Phase::Pace:
  return debug::RenderStage::Pace;
 case FramePipeline::Phase::Diagnostics:
  return debug::RenderStage::Diagnostics;
 }
 return debug::RenderStage::Diagnostics;
}
}
void FramePipeline::beginFrame() {
#ifdef MINECRAFT_FRAME_PROFILE
 records_.clear();
#endif
}
void FramePipeline::beginPhase(Phase phase) {
#ifdef MINECRAFT_FRAME_PROFILE
 currentPhase_ = phase;
 phaseStart_ = std::chrono::steady_clock::now();
#else
 (void)phase;
#endif
}
void FramePipeline::endPhase() {
#ifdef MINECRAFT_FRAME_PROFILE
 const std::chrono::microseconds duration =
     std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - phaseStart_);
 records_.push_back({currentPhase_, duration});
#endif
}
std::size_t FramePipeline::recordCount() const noexcept {
 return records_.size();
}
const std::vector<FramePipeline::Record>& FramePipeline::records() const noexcept {
 return records_;
}
void FramePipeline::run() {
 run({});
}
void FramePipeline::run(const PhaseTask& task) {
 net::minecraft::util::concurrent::FrameBudget::beginFrame(16);
 beginFrame();
 for(const Phase phase : kPhaseOrder) {
  beginPhase(phase);
  {
   const debug::RenderProfiler::Scope scope(profilerStage(phase));
   if(phase == Phase::Drain) (void)mailbox_.drainUrgent();
   if(phase == Phase::Ticks) (void)mailbox_.drainTick();
   if(phase == Phase::Render) (void)mailbox_.drainRender();
   if(task) task(phase);
  }
  endPhase();
 }
}
} // namespace net::minecraft::client::util
