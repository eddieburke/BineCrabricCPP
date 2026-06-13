#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/mod/ModLifecycle.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace net::minecraft::registry {

namespace {

struct QueuedInit {
    int order = 0;
    void (*fn)() = nullptr;
};

std::vector<QueuedInit>& phaseBucket(mod::LifecyclePhase phase)
{
    static std::array<std::vector<QueuedInit>, static_cast<std::size_t>(mod::LifecyclePhase::Frozen) + 1> buckets;
    return buckets[static_cast<std::size_t>(phase)];
}

std::unordered_set<int>& usedBlockIds()
{
    static std::unordered_set<int> ids;
    return ids;
}

std::unordered_set<int>& usedItemIds()
{
    static std::unordered_set<int> ids;
    return ids;
}

std::unordered_set<int>& usedEntityIds()
{
    static std::unordered_set<int> ids;
    return ids;
}

std::once_flag g_bootstrapFlag;
bool g_bootstrapped = false;

void runPhase(mod::LifecyclePhase phase)
{
    mod::ModLifecycle::advanceTo(phase);
    std::vector<QueuedInit>& bucket = phaseBucket(phase);
    std::stable_sort(bucket.begin(), bucket.end(), [](const QueuedInit& lhs, const QueuedInit& rhs) {
        return lhs.order < rhs.order;
    });
    for (const QueuedInit& entry : bucket) {
        entry.fn();
    }
}

} // namespace

void Registry::enqueue(mod::LifecyclePhase phase, int order, void (*initFunc)())
{
    phaseBucket(phase).push_back({order, initFunc});
}

void Registry::addBlock(int id, void (*initFunc)())
{
    assert(usedBlockIds().insert(id).second && "Registry: duplicate block id registration");
    enqueue(mod::LifecyclePhase::BlockRegistration, id, initFunc);
}

void Registry::addItem(int id, void (*initFunc)())
{
    assert(usedItemIds().insert(id).second && "Registry: duplicate item id registration");
    enqueue(mod::LifecyclePhase::ItemRegistration, id, initFunc);
}

void Registry::addEntity(int rawId, void (*initFunc)())
{
    assert(usedEntityIds().insert(rawId).second && "Registry: duplicate entity id registration");
    enqueue(mod::LifecyclePhase::EntityRegistration, rawId, initFunc);
}

void Registry::bootstrap()
{
    std::call_once(g_bootstrapFlag, [] {
        mod::ModLifecycle::advanceTo(mod::LifecyclePhase::BootstrapStarting);

        // Phases run in this fixed order. Add a phase here (and to the enum)
        // rather than hand-threading another runPhase() call.
        static constexpr mod::LifecyclePhase kOrder[] = {
            mod::LifecyclePhase::BlockRegistration,
            mod::LifecyclePhase::BlockRegistryFinalize,
            mod::LifecyclePhase::BiomeRegistration,
            mod::LifecyclePhase::ItemRegistration,
            mod::LifecyclePhase::BlockItemRegistration,
            mod::LifecyclePhase::SmeltingRecipeRegistration,
            mod::LifecyclePhase::CraftingRecipeRegistration,
            mod::LifecyclePhase::EntityRegistration,
            mod::LifecyclePhase::BlockEntityRegistration,
            mod::LifecyclePhase::DimensionRegistration,
            mod::LifecyclePhase::FuelRegistration,
            mod::LifecyclePhase::ClientRendererRegistration,
            mod::LifecyclePhase::ParticleRegistration,
        };
        for (const mod::LifecyclePhase phase : kOrder) {
            runPhase(phase);
        }

        g_bootstrapped = true;
        mod::ModLifecycle::advanceTo(mod::LifecyclePhase::Frozen);
    });
}

bool Registry::isBootstrapped()
{
    return g_bootstrapped;
}

} // namespace net::minecraft::registry
