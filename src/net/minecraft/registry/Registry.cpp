#include "net/minecraft/registry/Registry.hpp"

#include <algorithm>
#include <mutex>
#include <vector>
#include <iostream>

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

std::once_flag g_bootstrapFlag;
} // namespace

void Registry::addBlock(int id, void (*initFunc)()) {
    entries().push_back({initFunc, id});
}

void Registry::addItem(int id, void (*initFunc)()) {
    entries().push_back({initFunc, 1000 + id});
}

void Registry::addEntity(int rawId, void (*initFunc)()) {
    entries().push_back({initFunc, 30000 + rawId});
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
        
        std::cout << "Bootstrapping " << list.size() << " registered elements...\n";
        for (const Entry& entry : list) {
            entry.fn();
        }
    });
}

} // namespace net::minecraft::registry
