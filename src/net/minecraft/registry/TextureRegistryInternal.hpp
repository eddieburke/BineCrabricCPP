#pragma once
#include "net/minecraft/registry/TextureRegistry.hpp"
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace net::minecraft::registry::detail {

inline std::mutex& registryMutex() {
    static std::mutex m;
    return m;
}

inline std::deque<TextureRegistry::Entry>& registryEntries() {
    static std::deque<TextureRegistry::Entry> entries;
    return entries;
}

inline std::unordered_map<std::string, int>& registryIndex() {
    static std::unordered_map<std::string, int> index;
    return index;
}

inline std::unordered_set<int>& warnedInvalidIds() {
    static std::unordered_set<int> warned;
    return warned;
}

} // namespace net::minecraft::registry::detail
