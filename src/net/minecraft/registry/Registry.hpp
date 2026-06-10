#pragma once

#include <vector>

namespace net::minecraft::registry {

class Registry {
public:
    static void addBlock(int id, void (*init)());
    static void addItem(int id, void (*init)());
    static void addEntity(int id, void (*init)());
    static void addCustom(int priority, void (*init)());
    
    // Called once at startup to execute everything in order
    static void bootstrap();
};

template <typename T>
struct RegisterBlock {
    RegisterBlock(int id) { Registry::addBlock(id, T::registerClass); }
};

template <typename T>
struct RegisterItem {
    RegisterItem(int id) { Registry::addItem(id, T::registerClass); }
};

template <typename T>
struct RegisterEntity {
    RegisterEntity(int id) { Registry::addEntity(id, T::registerClass); }
};

template <typename T>
struct RegisterCustom {
    RegisterCustom(int priority) { Registry::addCustom(priority, T::registerClass); }
};

} // namespace net::minecraft::registry
