#include "net/minecraft/registry/Registry.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <mutex>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/ContentRegistries.hpp"
namespace net::minecraft::registry {
namespace {
struct QueuedInit {
  int order = 0;
  void (*fn)() = nullptr;
};
std::vector<QueuedInit>& phaseBucket(mod::LifecyclePhase phase) {
  static std::array<std::vector<QueuedInit>, static_cast<std::size_t>(mod::LifecyclePhase::Ready) + 1> buckets;
  return buckets[static_cast<std::size_t>(phase)];
}
std::unordered_set<int>& usedBlockIds() {
  static std::unordered_set<int> ids;
  return ids;
}
std::unordered_set<int>& usedItemIds() {
  static std::unordered_set<int> ids;
  return ids;
}
std::unordered_set<int>& usedEntityIds() {
  static std::unordered_set<int> ids;
  return ids;
}
std::once_flag g_bootstrapFlag;
bool g_bootstrapped = false;
void verifyBootstrapState() {
  if(block::Block::GRASS_BLOCK == nullptr) {
    throw std::runtime_error("Registry bootstrap failed: Block::GRASS_BLOCK is null");
  }
  if(entity::EntityRegistry::create("Zombie", nullptr) == nullptr) {
    throw std::runtime_error("Registry bootstrap failed: entity id 'Zombie' missing");
  }
  if(!BlockEntityRegistry::instance().hasFactory("Chest")) {
    throw std::runtime_error("Registry bootstrap failed: block entity id 'Chest' missing");
  }
}
void runPhase(mod::LifecyclePhase phase) {
  mod::ModLifecycle::advanceTo(phase);
  std::vector<QueuedInit>& bucket = phaseBucket(phase);
  while(!bucket.empty()) {
    const auto next = std::min_element(bucket.begin(), bucket.end(), [](const QueuedInit& lhs, const QueuedInit& rhs) {
      return lhs.order < rhs.order;
    });
    const QueuedInit entry = *next;
    bucket.erase(next);
    if(entry.fn != nullptr) {
      entry.fn();
    }
  }
  // Sort once after every callback in this phase, including Lua mod batches at order 50000.
  if(phase == mod::LifecyclePhase::PostInit) {
    recipe::CraftingRecipeManager::getInstance().finishRegistration();
  }
}
} // namespace
void Registry::enqueue(mod::LifecyclePhase phase, int order, void (*initFunc)()) {
  phaseBucket(phase).push_back({order, initFunc});
}
bool Registry::tryReserveBlockId(int id) {
  return usedBlockIds().insert(id).second;
}
bool Registry::tryReserveItemId(int rawId) {
  return usedItemIds().insert(rawId).second;
}
void Registry::reserveBlockId(int id) {
  assert(tryReserveBlockId(id) && "Registry: duplicate block id registration");
}
void Registry::addBlock(int id, void (*initFunc)()) {
  reserveBlockId(id);
  enqueue(mod::LifecyclePhase::Init, id, initFunc);
}
void Registry::addItem(int id, void (*initFunc)()) {
  assert(usedItemIds().insert(id).second && "Registry: duplicate item id registration");
  enqueue(mod::LifecyclePhase::Init, id, initFunc);
}
void Registry::addEntity(int rawId, void (*initFunc)()) {
  assert(usedEntityIds().insert(rawId).second && "Registry: duplicate entity id registration");
  enqueue(mod::LifecyclePhase::Init, rawId, initFunc);
}
void Registry::bootstrap() {
  std::call_once(g_bootstrapFlag, [] {
    // Load Lua zip mods before content phases run so at_phase registrations and
    // hook subscriptions are live for the rest of the session.
    mod::runtime::host().initialize(mod::runtime::ModHost::defaultRunDirectory());
    mod::runtime::host().loadEnabledPackageMods();
    // Phases run in this fixed order.
    static constexpr mod::LifecyclePhase kOrder[] = {
        mod::LifecyclePhase::Init,
        mod::LifecyclePhase::PostInit,
    };
    for(const mod::LifecyclePhase phase : kOrder) {
      runPhase(phase);
    }
    g_bootstrapped = true;
    verifyBootstrapState();
    mod::ModLifecycle::advanceTo(mod::LifecyclePhase::Ready);
  });
}
bool Registry::isBootstrapped() {
  return g_bootstrapped;
}
} // namespace net::minecraft::registry
