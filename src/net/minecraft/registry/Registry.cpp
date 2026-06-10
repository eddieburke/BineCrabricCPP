#include "net/minecraft/registry/Registry.hpp"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace net::minecraft::registry {

namespace {
struct Entry {
    void (*fn)();
    int priority;
};

std::vector<Entry>& entries() {
    static std::vector<Entry> list;
    return list;
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
} // namespace

void Registry::addBlock(int id, void (*initFunc)()) {
    assert(usedBlockIds().insert(id).second && "Registry: duplicate block id registration");
    entries().push_back({initFunc, kBlockRegistrarBase + id});
}

void Registry::addItem(int id, void (*initFunc)()) {
    assert(usedItemIds().insert(id).second && "Registry: duplicate item id registration");
    entries().push_back({initFunc, kItemRegistrarBase + id});
}

void Registry::addEntity(int rawId, void (*initFunc)()) {
    assert(usedEntityIds().insert(rawId).second && "Registry: duplicate entity id registration");
    entries().push_back({initFunc, kEntityRegistrarBase + rawId});
}

void Registry::addCustom(int priority, void (*initFunc)()) {
    entries().push_back({initFunc, priority});
}

void Registry::bootstrap() {
    std::call_once(g_bootstrapFlag, [] {
        auto& list = entries();
        std::sort(list.begin(), list.end(), [](const Entry& a, const Entry& b) {
            return a.priority < b.priority;
        });

        for (const Entry& entry : list) {
            entry.fn();
        }
    });
}

} // namespace net::minecraft::registry
