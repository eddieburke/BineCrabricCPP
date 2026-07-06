#include "net/minecraft/client/lifecycle/ClientShutdown.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/MinecraftApplet.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include "net/minecraft/client/util/DisplayManager.hpp"
#include "net/minecraft/client/util/GlAllocationUtils.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/world/chunk/storage/RegionIo.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <typeinfo>
#if defined(__GNUC__)
#include <cxxabi.h>
#endif
#ifdef _WIN32
#include "net/minecraft/client/input/InputSystem.hpp"
#endif
namespace net::minecraft::client::lifecycle {
namespace {
// A bare catch(...) ("unknown error") tells us nothing. Recover the in-flight
// exception's dynamic type so a non-std::exception throw is named in the log
// instead of being swallowed silently. A null type means a foreign exception
// (not a C++ throw) reached us — e.g. a hardware fault forwarded by the runtime.
std::string currentExceptionName() {
#if defined(__GNUC__)
  const std::type_info* type = abi::__cxa_current_exception_type();
  if(type == nullptr) {
    return "foreign/non-C++ exception (likely a hardware fault, e.g. access violation)";
  }
  int status = 0;
  char* demangled = abi::__cxa_demangle(type->name(), nullptr, nullptr, &status);
  std::string name = (status == 0 && demangled != nullptr) ? demangled : type->name();
  std::free(demangled);
  return name;
#else
  return "unknown error";
#endif
}
} // namespace
void ClientShutdown::stop(Minecraft& client) {
  try {
    if(client.stats != nullptr) {
      client.stats->syncStats();
      client.stats->save();
    }
    if(client.applet != nullptr) {
      client.applet->clearMemory();
    }
    try {
      if(client.resourceDownloadThread != nullptr) {
        client.resourceDownloadThread->cancel();
      }
    } catch(...) {
    }
    std::cout << "Stopping!" << std::endl;
    try {
      if(client.world != nullptr) {
        client.world->savingProgress(nullptr);
      }
    } catch(const std::exception& e) {
      std::cerr << "Save on shutdown failed: " << e.what() << '\n';
    } catch(...) {
      std::cerr << "Save on shutdown failed: " << currentExceptionName() << '\n';
    }
    try {
      client.setWorld(nullptr);
    } catch(const std::exception& e) {
      std::cerr << "World teardown failed: " << e.what() << '\n';
    } catch(...) {
      std::cerr << "World teardown failed: " << currentExceptionName() << '\n';
    }
    // Defensive: flush and close any region files still open before std::_Exit (below)
    // skips their destructors. The normal teardown already flushed them via the synchronous
    // world save in setWorld(nullptr); this covers a throw mid-teardown that leaves writes
    // buffered. No-op once RegionIo's open-file map has been cleared.
    try {
      RegionIo::flush();
    } catch(...) {
    }
    try {
      util::GlAllocationUtils::clear();
    } catch(...) {
    }
    client.audio.shutdown();
#ifdef _WIN32
    input::InputSystem::shutdown();
#endif
  } catch(...) {
  }
#ifdef _WIN32
  util::DisplayManager::destroy();
#endif
  if(!client.crashed) {
    std::cout.flush();
    std::_Exit(0);
  }
}
void ClientShutdown::cleanHeap(Minecraft& client) {
  try {
    Minecraft::MEMORY_RESERVED_FOR_CRASH.clear();
    if(client.worldRenderer != nullptr) {
      client.worldRenderer->releaseSections();
    }
  } catch(...) {
  }
  try {
    client.setWorld(nullptr);
  } catch(...) {
  }
}
} // namespace net::minecraft::client::lifecycle
