#pragma once
#include <set>
#include <stdexcept>
#include <unordered_set>
#include "net/minecraft/world/BlockEvent.hpp"
namespace net::minecraft {
// Scheduled block tick queue extracted from World. Mirrors Java's paired
// TreeSet (time-ordered iteration) + HashSet (O(1) dedup) for TickNextTick:
// both containers always hold the same events.
class ScheduledTickQueue {
 public:
 // Dedup insert: ignored if an equal event (same pos + blockId) is queued.
 void schedule(const BlockEvent& event) {
  if(members_.insert(event).second) {
   ordered_.insert(event.withTicks(event.ticks - shift_));
  }
 }
 [[nodiscard]] bool empty() const noexcept {
  return ordered_.empty();
 }
 // Java parity: TickNextTick processes at most 1000 events per tick and
 // throws if the paired containers ever diverge.
 [[nodiscard]] int tickBudget() const {
  if(ordered_.size() != members_.size()) {
   throw std::runtime_error("TickNextTick list out of synch");
  }
  int budget = static_cast<int>(ordered_.size());
  return budget > 1000 ? 1000 : budget;
 }
 [[nodiscard]] BlockEvent peek() const {
  const BlockEvent& event = *ordered_.begin();
  return event.withTicks(event.ticks + shift_);
 }
 void popFront() {
  members_.erase(*ordered_.begin());
  ordered_.erase(ordered_.begin());
 }
 // ordered_ holds shift_-relative times. A uniform delta moves every queued event by
 // the same amount, so it changes neither the (ticks, globalId) order of ordered_ nor
 // members_, whose hash and equality read only pos + blockId. Rebuilding both
 // containers to apply it was 22.8% of all CPU under a mod that drives the clock.
 void shiftScheduledTimes(long long delta) noexcept {
  shift_ += delta;
 }

 private:
 std::set<BlockEvent, BlockEventComparator> ordered_;
 std::unordered_set<BlockEvent, BlockEventHash> members_;
 long long shift_ = 0;
};
} // namespace net::minecraft
