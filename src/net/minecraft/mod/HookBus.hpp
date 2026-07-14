#pragma once
#include <algorithm>
#include <functional>
#include <utility>
#include <vector>
#include "net/minecraft/mod/MinecraftApi.hpp"
namespace net::minecraft::mod {
namespace detail {
using HookOwnerCleaner = void (*)(const void*);
inline std::vector<HookOwnerCleaner>& hookOwnerCleaners() {
  static std::vector<HookOwnerCleaner> value;
  return value;
}
inline void registerHookOwnerCleaner(HookOwnerCleaner cleaner) {
  hookOwnerCleaners().push_back(cleaner);
}
} // namespace detail
template <typename Event>
class HookList {
public:
  using Listener = std::function<void(Event&)>;
  static void add(int priority, Listener listener) {
    addOwned(nullptr, priority, std::move(listener));
  }
  static void addOwned(const void* owner, int priority, Listener listener) {
    ensureOwnerCleanerRegistered();
    listeners().push_back({owner, priority, std::move(listener)});
    std::stable_sort(listeners().begin(), listeners().end(), [](const Entry& lhs, const Entry& rhs) {
      return lhs.priority < rhs.priority;
    });
  }
  static void removeOwner(const void* owner) {
    if(owner == nullptr) {
      return;
    }
    auto& values = listeners();
    values.erase(std::remove_if(values.begin(), values.end(), [owner](const Entry& entry) {
                   return entry.owner == owner;
                 }),
                 values.end());
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
    const void* owner = nullptr;
    int priority = 0;
    Listener listener;
  };
  static void ensureOwnerCleanerRegistered() {
    static const bool registered = [] {
      detail::registerHookOwnerCleaner(&HookList<Event>::removeOwner);
      return true;
    }();
    (void)registered;
  }
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
  void subscribeOwned(const void* owner, int priority, typename HookList<Event>::Listener listener) {
    HookList<Event>::addOwned(owner, priority, std::move(listener));
  }
  void unsubscribeOwner(const void* owner) const {
    for(const detail::HookOwnerCleaner cleaner : detail::hookOwnerCleaners()) {
      cleaner(owner);
    }
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
