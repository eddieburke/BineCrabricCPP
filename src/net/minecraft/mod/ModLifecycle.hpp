#pragma once
#include "net/minecraft/mod/HookBus.hpp"
namespace net::minecraft::mod {
// Lifecycle phases run once at startup via Registry::bootstrap().
// Lua mods register content with minecraft.at_phase() during script load.
// Runtime hooks use minecraft.on(event, callback).
enum class LifecyclePhase {
  NotStarted,
  BootstrapStarting,
  BiomeRegistration,
  BlockRegistration,
  BlockRegistryFinalize,
  ItemRegistration,
  BlockItemRegistration,
  SmeltingRecipeRegistration,
  CraftingRecipeRegistration,
  EntityRegistration,
  BlockEntityRegistration,
  FuelRegistration,
  ClientRendererRegistration,
  ParticleRegistration,
  Frozen
};
struct LifecycleEvent {
  LifecyclePhase previous = LifecyclePhase::NotStarted;
  LifecyclePhase current = LifecyclePhase::NotStarted;
};
class ModLifecycle {
public:
  [[nodiscard]] static LifecyclePhase currentPhase() noexcept {
    return phaseStorage();
  }
  [[nodiscard]] static bool frozen() noexcept {
    return phaseStorage() == LifecyclePhase::Frozen;
  }
  static void advanceTo(LifecyclePhase phase) {
    LifecycleEvent event{phaseStorage(), phase};
    phaseStorage() = phase;
    hooks().publish(event);
  }

private:
  static LifecyclePhase& phaseStorage() noexcept {
    static LifecyclePhase phase = LifecyclePhase::NotStarted;
    return phase;
  }
};
} // namespace net::minecraft::mod
