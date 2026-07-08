#pragma once
// Factory registry mapping a PersistentState subtype to a constructor so SavedDataStorage can
// rebuild it when loading from disk. Replaces the old hardcoded `typeid(MapState)` dispatch:
// any type registered here — engine or mod — round-trips through the world save.
//
// `World::getOrCreateState<T>` registers T on first use, so mods persist fully-custom NBT with
// zero extra wiring. Types that are also loaded through the type-erased path (e.g. MapItem
// looking up an existing map without ever templating getOrCreateState) register explicitly via
// a static initializer (see MapState.cpp).
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "net/minecraft/world/PersistentState.hpp"

namespace net::minecraft {
class PersistentStateRegistry {
   public:
    using Factory = std::function<std::unique_ptr<PersistentState>(const std::string&)>;

    static PersistentStateRegistry& instance() {
        static PersistentStateRegistry registry;
        return registry;
    }

    // Idempotent. StateT must derive from PersistentState and be constructible from its id string.
    template <typename StateT>
    void registerType() {
        const std::type_index key(typeid(StateT));
        if (factories_.find(key) == factories_.end()) {
            factories_.emplace(key, [](const std::string& id) -> std::unique_ptr<PersistentState> {
                return std::make_unique<StateT>(id);
            });
        }
    }

    // Returns nullptr if the type was never registered (the caller then keeps its prior behavior
    // of constructing a fresh, empty state).
    [[nodiscard]] std::unique_ptr<PersistentState> create(const std::type_index& type, const std::string& id) const {
        const auto it = factories_.find(type);
        if (it == factories_.end()) {
            return nullptr;
        }
        return it->second(id);
    }

   private:
    std::unordered_map<std::type_index, Factory> factories_;
};
}  // namespace net::minecraft
