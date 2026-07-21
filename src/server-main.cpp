#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/ServerLog.hpp"
#include "net/minecraft/server/host/ServerLaunchConfig.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#ifdef _WIN32
#include "net/minecraft/server/dedicated/gui/DedicatedServerGui.hpp"
#include <windows.h>
#include <timeapi.h>
struct WindowsTimerResolutionReserver {
 WindowsTimerResolutionReserver() {
  timeBeginPeriod(1);
 }
 ~WindowsTimerResolutionReserver() {
  timeEndPeriod(1);
 }
};
#endif
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
namespace {
[[nodiscard]] bool hasNoGuiArg(int argc, char** argv) {
 for(int i = 1; i < argc; ++i) {
  if(argv[i] != nullptr && std::string(argv[i]) == "nogui") {
   return true;
  }
 }
 return false;
}
[[nodiscard]] bool parseBool(const std::string& value, const std::string& option) {
 if(value == "true") {
  return true;
 }
 if(value == "false") {
  return false;
 }
 throw std::invalid_argument(option + " expects true or false");
}
[[nodiscard]] std::optional<net::minecraft::server::host::ServerLaunchConfig> parseLaunchConfig(int argc,
                                                                                                char** argv) {
 using net::minecraft::server::host::ServerLaunchConfig;
 ServerLaunchConfig config;
 bool configured = false;
 for(int i = 1; i < argc; ++i) {
  const std::string option = argv[i] == nullptr ? std::string{} : argv[i];
  if(option == "nogui") {
   continue;
  }
  if(i + 1 >= argc || argv[i + 1] == nullptr) {
   throw std::invalid_argument("Missing value for " + option);
  }
  const std::string value = argv[++i];
  configured = true;
  if(option == "--storage-root") {
   config.storageRoot = std::filesystem::path(value);
  } else if(option == "--level-name") {
   config.worldName = value;
  } else if(option == "--level-seed") {
   config.worldSeed = static_cast<std::uint64_t>(std::stoull(value));
  } else if(option == "--server-ip") {
   config.bindAddress = value;
  } else if(option == "--server-port") {
   config.port = std::stoi(value);
  } else if(option == "--ready-file") {
   config.readyFile = std::filesystem::path(value);
  } else if(option == "--online-mode") {
   config.onlineMode = parseBool(value, option);
  } else if(option == "--spawn-animals") {
   config.spawnAnimals = parseBool(value, option);
  } else if(option == "--pvp") {
   config.pvpEnabled = parseBool(value, option);
  } else if(option == "--allow-flight") {
   config.flightEnabled = parseBool(value, option);
  } else if(option == "--allow-nether") {
   config.allowNether = parseBool(value, option);
  } else if(option == "--mods-enabled") {
   config.modsEnabled = parseBool(value, option);
  } else {
   throw std::invalid_argument("Unknown server option: " + option);
  }
 }
 if(!configured) {
  return std::nullopt;
 }
 if(config.storageRoot.empty() || config.worldName.empty()) {
  throw std::invalid_argument("--storage-root and --level-name are required for configured launches");
 }
 if(config.port < 1 || config.port > 65535) {
  throw std::invalid_argument("--server-port must be from 1 to 65535");
 }
 config.useConsoleThread = true;
 config.useGui = false;
 return config;
}
} // namespace
int main(int argc, char** argv) {
#ifdef _WIN32
 WindowsTimerResolutionReserver timerReserver;
#endif
 using net::minecraft::server::LogLevel;
 using net::minecraft::server::MinecraftServer;
 using net::minecraft::server::ServerLog;
 using net::minecraft::stat::Stats;
 try {
  const auto launchConfig = parseLaunchConfig(argc, argv);
  net::minecraft::mod::runtime::host().setRuntimeSide(net::minecraft::mod::runtime::ModRuntimeSide::Server);
  net::minecraft::mod::runtime::host().setPackageLoadingEnabled(!launchConfig.has_value() || launchConfig->modsEnabled);
  Stats::initialize();
  std::unique_ptr<MinecraftServer> server = launchConfig.has_value()
                                                ? std::make_unique<MinecraftServer>(*launchConfig)
                                                : std::make_unique<MinecraftServer>();
#ifdef _WIN32
  if(!hasNoGuiArg(argc, argv)) {
   net::minecraft::server::dedicated::gui::DedicatedServerGui::create(*server);
  }
#endif
  std::thread serverThread([&server]() { server->run(); });
  serverThread.join();
 } catch(const std::exception& exception) {
  ServerLog::LOGGER.log(LogLevel::Severe, "Failed to start the minecraft server", &exception);
  return 1;
 }
 return 0;
}
