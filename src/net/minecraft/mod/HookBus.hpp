#pragma once

#include <algorithm>
#include <functional>
#include <vector>

namespace net::minecraft::mod {

enum class HookFlow {
    Continue,
    Stop
};

struct HookResult {
    HookFlow flow = HookFlow::Continue;

    [[nodiscard]] static HookResult continueHooks() noexcept { return {HookFlow::Continue}; }
    [[nodiscard]] static HookResult stopHooks() noexcept { return {HookFlow::Stop}; }
    [[nodiscard]] bool shouldStop() const noexcept { return flow == HookFlow::Stop; }
};

template <typename Event>
class HookList {
public:
    using Listener = std::function<HookResult(Event&)>;

    static void add(int priority, Listener listener)
    {
        listeners().push_back({priority, std::move(listener)});
        std::stable_sort(listeners().begin(), listeners().end(), [](const Entry& lhs, const Entry& rhs) {
            return lhs.priority < rhs.priority;
        });
    }

    static HookResult publish(Event& event)
    {
        for (const Entry& entry : listeners()) {
            HookResult result = entry.listener(event);
            if (result.shouldStop()) {
                return result;
            }
        }
        return HookResult::continueHooks();
    }

    [[nodiscard]] static bool empty()
    {
        return listeners().empty();
    }

private:
    struct Entry {
        int priority = 0;
        Listener listener;
    };

    static std::vector<Entry>& listeners()
    {
        static std::vector<Entry> value;
        return value;
    }
};

class HookBus {
public:
    template <typename Event>
    void subscribe(int priority, typename HookList<Event>::Listener listener)
    {
        HookList<Event>::add(priority, std::move(listener));
    }

    template <typename Event>
    HookResult publish(Event& event) const
    {
        return HookList<Event>::publish(event);
    }

    template <typename Event>
    [[nodiscard]] bool hasListeners() const
    {
        return !HookList<Event>::empty();
    }
};

inline HookBus& hooks()
{
    static HookBus bus;
    return bus;
}

} // namespace net::minecraft::mod
