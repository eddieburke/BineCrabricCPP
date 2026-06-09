#include "net/minecraft/registry/VanillaRegistry.hpp"

#include <mutex>

namespace net::minecraft::registry {

namespace {
std::once_flag g_vanillaBootstrap;
} // namespace

void runVanillaBootstrap()
{
    std::call_once(g_vanillaBootstrap, [] {
        AutoRegistry<VanillaRegistryTag>::runAll();
    });
}

} // namespace net::minecraft::registry
