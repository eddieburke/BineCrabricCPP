#pragma once
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/lua/LuaModNaming.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::mod::lua {
// Shared id->spec store for Lua-registered content. Traits supply:
//   using Spec = ...;                        // must expose translationKey/displayName
//   static constexpr const char* kKind;      // "block" / "item" (alias + wire-name prefix)
//   static constexpr mod::LifecyclePhase kPhase;
//   static void instantiate(const Spec&);
template <typename Traits>
class ModIdRegistry {
   public:
    using Spec = typename Traits::Spec;

    static ModIdRegistry& instance() {
        static ModIdRegistry value;
        return value;
    }

    [[nodiscard]] bool contains(int id) const {
        return specs_.find(id) != specs_.end();
    }

    void add(int id, const Spec& spec) {
        specs_[id] = spec;
        registerAliases(id, spec);
        if (!queued_) {
            queued_ = true;
            registry::Registry::enqueue(Traits::kPhase, 50000, &ModIdRegistry::initAll);
        }
    }

    [[nodiscard]] int idFromName(const char* name) const {
        if (name == nullptr || *name == '\0') {
            return 0;
        }
        const auto it = names_.find(normalizeNameToken(name));
        return it == names_.end() ? 0 : it->second;
    }

    [[nodiscard]] std::string wireName(int id) const {
        const auto it = specs_.find(id);
        if (it == specs_.end()) {
            return {};
        }
        const std::string keyToken = wireToken(id, it->second);
        const std::string snake = toSnakeCase(keyToken);
        return snake.empty() ? keyToken : snake;
    }

    [[nodiscard]] const Spec* specForId(int id) const noexcept {
        const auto it = specs_.find(id);
        return it == specs_.end() ? nullptr : &it->second;
    }

   private:
    static std::string wireToken(int id, const Spec& spec) {
        return spec.translationKey.empty() ? Traits::kKind + std::to_string(id) : spec.translationKey;
    }

    static void initAll() {
        auto& specs = instance().specs_;
        std::vector<int> ids;
        ids.reserve(specs.size());
        for (const auto& entry : specs) {
            ids.push_back(entry.first);
        }
        std::sort(ids.begin(), ids.end());
        for (const int id : ids) {
            const auto it = specs.find(id);
            if (it != specs.end()) {
                Traits::instantiate(it->second);
            }
        }
    }

    void registerAliases(int id, const Spec& spec) {
        auto put = [&](const std::string& alias) {
            const std::string key = normalizeNameToken(alias);
            if (!key.empty()) {
                names_[key] = id;
            }
        };
        const std::string keyToken = wireToken(id, spec);
        put(keyToken);
        const std::string snake = toSnakeCase(keyToken);
        if (!snake.empty()) {
            put(snake);
            put(snake + "s");
        }
        put(std::string(Traits::kKind) + "_" + std::to_string(id));
        if (!spec.displayName.empty()) {
            put(spec.displayName);
        }
    }

    std::unordered_map<int, Spec> specs_;
    std::unordered_map<std::string, int> names_;
    bool queued_ = false;
};
}  // namespace net::minecraft::mod::lua
