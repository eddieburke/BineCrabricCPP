#pragma once
#include "net/minecraft/mod/MinecraftApi.hpp"
#include <algorithm>
#include <functional>
#include <vector>
namespace net::minecraft::mod {
template <typename Event>
class HookList {
public:
  using Listener = std::function<void(Event&)>;
  static void add(int priority, Listener listener) {
    listeners().push_back({priority, std::move(listener)});
    std::stable_sort(listeners().begin(), listeners().end(),
                     [](const Entry& lhs, const Entry& rhs) { return lhs.priority < rhs.priority; });
  }
  static void publish(Event& event) {
    for(const Entry& entry : listeners()) {
      entry.listener(event);
    }
  }
  [[nodiscard]] static bool empty() {
    return listeners().empty();
  }

private:
  struct Entry {
    int priority = 0;
    Listener listener;
  };
  static std::vector<Entry>& listeners() {
    static std::vector<Entry> value;
    return value;
  }
};
class HookBus {
public:
  template <typename Event>
  void subscribe(int priority, typename HookList<Event>::Listener listener) {
    HookList<Event>::add(priority, std::move(listener));
  }
  template <typename Event>
  void publish(Event& event) const {
    HookList<Event>::publish(event);
  }
  template <typename Event>
  [[nodiscard]] bool hasListeners() const {
    return !HookList<Event>::empty();
  }
};
inline HookBus& hooks() {
  static HookBus bus;
  return bus;
}
} // namespace net::minecraft::mod
