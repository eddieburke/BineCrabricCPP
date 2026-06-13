#include "net/minecraft/client/lifecycle/ClientShutdown.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/MinecraftApplet.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include "net/minecraft/client/util/DisplayManager.hpp"
#include "net/minecraft/client/util/GlAllocationUtils.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"

#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#include "net/minecraft/client/input/InputSystem.hpp"
#endif

namespace net::minecraft::client::lifecycle {

void ClientShutdown::stop(Minecraft& client)
{
    try {
        if (client.stats != nullptr) {
            client.stats->syncStats();
            client.stats->save();
        }
        if (client.applet != nullptr) {
            client.applet->clearMemory();
        }
        try {
            if (client.resourceDownloadThread != nullptr) {
                client.resourceDownloadThread->cancel();
            }
        } catch (...) {
        }
        std::cout << "Stopping!" << std::endl;
        try {
            client.setWorld(nullptr);
        } catch (...) {
        }
        try {
            util::GlAllocationUtils::clear();
        } catch (...) {
        }
        client.audio.shutdown();
#ifdef _WIN32
        input::InputSystem::shutdown();
#endif
    } catch (...) {
    }
#ifdef _WIN32
    util::DisplayManager::destroy();
#endif
    if (!client.crashed) {
        std::cout.flush();
        std::_Exit(0);
    }
}

void ClientShutdown::cleanHeap(Minecraft& client)
{
    try {
        Minecraft::MEMORY_RESERVED_FOR_CRASH.clear();
        if (client.worldRenderer != nullptr) {
            client.worldRenderer->releaseGlLists();
        }
        if (client.atmosphereRenderer != nullptr) {
            client.atmosphereRenderer->releaseGpuResources();
        }
    } catch (...) {
    }
    try {
        client.setWorld(nullptr);
    } catch (...) {
    }
}

} // namespace net::minecraft::client::lifecycle
