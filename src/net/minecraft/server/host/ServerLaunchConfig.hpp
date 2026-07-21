#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
namespace net::minecraft::server::host {
struct ServerLaunchConfig {
 std::filesystem::path storageRoot;
 std::string worldName;
 std::uint64_t worldSeed = 0;
 std::string bindAddress;
 int port = 25565;
 std::filesystem::path readyFile;
 bool onlineMode = false;
 bool spawnAnimals = true;
 bool pvpEnabled = true;
 bool flightEnabled = false;
 bool allowNether = true;
 bool useConsoleThread = false;
 bool useGui = false;
 bool modsEnabled = true;
};
} // namespace net::minecraft::server::host
