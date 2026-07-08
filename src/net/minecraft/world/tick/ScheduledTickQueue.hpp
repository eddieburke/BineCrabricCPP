#pragma once
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "net/minecraft/world/BlockEvent.hpp"

namespace net::minecraft {
// Scheduled block tick queue extracted from World. Mirrors Java's paired
// TreeSet (time-ordered iteration) + HashSet (O(1) dedup) for TickNextTick:
// both containers always hold the same events.
class ScheduledTickQueue {
   public:
    // Dedup insert: ignored if an equal event (same pos + blockId) is queued.
    void schedule(const BlockEvent& event) {
        if (!members_.contains(event)) {
            members_.insert(event);
            ordered_.insert(event);
        }
    }

    [[nodiscard]] bool empty() const noexcept {
        return ordered_.empty();
    }

    // Java parity: TickNextTick processes at most 1000 events per tick and
    // throws if the paired containers ever diverge.
    [[nodiscard]] int tickBudget() const {
        if (ordered_.size() != members_.size()) {
            throw std::runtime_error("TickNextTick list out of synch");
        }
        int budget = static_cast<int>(ordered_.size());
        return budget > 1000 ? 1000 : budget;
    }

    [[nodiscard]] const BlockEvent& peek() const {
        return *ordered_.begin();
    }

    void popFront() {
        const BlockEvent event = *ordered_.begin();
        ordered_.erase(ordered_.begin());
        members_.erase(event);
    }

    void shiftScheduledTimes(long long delta) {
        if (delta == 0 || ordered_.empty()) {
            return;
        }
        std::vector<BlockEvent> events;
        events.reserve(ordered_.size());
        for (const BlockEvent& event : ordered_) {
            events.push_back(event.withTicks(event.ticks + delta));
        }
        ordered_.clear();
        members_.clear();
        for (const BlockEvent& event : events) {
            schedule(event);
        }
    }

   private:
    std::set<BlockEvent, BlockEventComparator> ordered_;
    std::unordered_set<BlockEvent, BlockEventHash> members_;
};
}  // namespace net::minecraft
