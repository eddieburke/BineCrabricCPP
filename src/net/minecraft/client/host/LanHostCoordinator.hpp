#pragma once
#include "net/minecraft/client/host/LanAddressResolver.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
namespace net::minecraft {
class World;
}
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::server {
class MinecraftServer;
}
namespace net::minecraft::client::host {
class LanHostCoordinator {
public:
  explicit LanHostCoordinator(client::Minecraft* minecraft);
  ~LanHostCoordinator();
  [[nodiscard]] static bool canHostWorld(const World* world);
  [[nodiscard]] bool canOpenLan() const;
  [[nodiscard]] bool isAwaitingLoopback() const noexcept;
  [[nodiscard]] bool isHosting() const noexcept;
  [[nodiscard]] bool isHostedWorld(const World* world) const noexcept;
  [[nodiscard]] std::uint16_t requestedPort() const noexcept;
  [[nodiscard]] std::uint16_t boundPort() const noexcept;
  [[nodiscard]] const LanConnectionInfo& connectionInfo() const noexcept;
  [[nodiscard]] const std::string& lastError() const noexcept;
  /// Parks the local world and starts the integrated server on a worker thread.
  bool beginHosting(std::uint16_t port, std::string& errorOut);
  /// Poll server startup from the main loop. Returns true once startup finished.
  bool tickHosting(std::string& errorOut);
  [[nodiscard]] bool isStartingServer() const noexcept;
  void onConnectCanceledOrFailed(const std::string& error = {});
  void afterWorldChange(World* world);
  void shutdown();

private:
  enum class State {
    Inactive,
    StartingServer,
    AwaitingLoopback,
    Active,
  };
  struct ServerStartResult {
    bool done = false;
    bool ok = false;
    std::string error;
    std::uint16_t boundPort = 0;
  };
  void stopServer();
  void clearState(bool clearError);
  void finishServerStart(bool ok, std::uint16_t boundPort, std::string error);
  client::Minecraft* minecraft_ = nullptr;
  std::unique_ptr<server::MinecraftServer> server_;
  std::thread serverStartThread_;
  std::mutex serverStartMutex_;
  ServerStartResult serverStartResult_;
  World* hostedRemoteWorld_ = nullptr;
  std::filesystem::path storageRoot_;
  std::string worldName_;
  std::uint64_t worldSeed_ = 0;
  std::uint16_t requestedPort_ = 0;
  std::uint16_t boundPort_ = 0;
  LanConnectionInfo connectionInfo_;
  std::string lastError_;
  State state_ = State::Inactive;
};
} // namespace net::minecraft::client::host
