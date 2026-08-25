#include "net/minecraft/client/util/FramePipeline.hpp"
#include "net/minecraft/util/concurrent/FrameBudget.hpp"
namespace net::minecraft::client::util {
void FramePipeline::run() {
 run({});
}
void FramePipeline::run(const PhaseTask& task) {
 net::minecraft::util::concurrent::FrameBudget::beginFrame(16);
 for(const Phase phase : kPhaseOrder) {
  if(phase == Phase::Drain) (void)mailbox_.drainUrgent();
  if(phase == Phase::Ticks) (void)mailbox_.drainTick();
  if(phase == Phase::Render) (void)mailbox_.drainRender();
  if(task) task(phase);
 }
}
} // namespace net::minecraft::client::util
