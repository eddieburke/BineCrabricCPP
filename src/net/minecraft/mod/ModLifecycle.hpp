#pragma once
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
namespace net::minecraft::mod {
class ModLifecycle {
 public:
 [[nodiscard]] static LifecyclePhase currentPhase() noexcept {
  return phaseStorage();
 }
 [[nodiscard]] static bool frozen() noexcept {
  return phaseStorage() == LifecyclePhase::Ready;
 }
 static void advanceTo(LifecyclePhase phase) {
  LifecycleEvent event{phaseStorage(), phase};
  phaseStorage() = phase;
  net::minecraft::mod::runtime::fireLifecycle(event.previous, event.current);
 }

 private:
 static LifecyclePhase& phaseStorage() noexcept {
  static LifecyclePhase phase = LifecyclePhase::NotStarted;
  return phase;
 }
};
} // namespace net::minecraft::mod
