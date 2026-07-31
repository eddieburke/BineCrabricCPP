#pragma once
#include <filesystem>
#include "net/minecraft/mod/runtime/ModHost.hpp"
namespace net::minecraft::test {
inline void initializeEmptyServerModHost(const std::filesystem::path& runDirectory) {
 auto& modHost = mod::runtime::host();
 modHost.shutdown();
 modHost.setRuntimeSide(mod::runtime::ModRuntimeSide::Server);
 modHost.setPackageLoadingEnabled(true);
 modHost.initialize(runDirectory);
 modHost.loadEnabledPackageMods();
}
} // namespace net::minecraft::test
