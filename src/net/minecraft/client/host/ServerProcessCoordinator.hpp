#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include "net/minecraft/client/host/LanAddressResolver.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::host {
struct ServerProcessSettings {
  std::uint16_t port = 25565;
  bool onlineMode = false;
  bool spawnAnimals = true;
  bool pvpEnabled = true;
  bool flightEnabled = false;
  bool allowNether = true;
  bool modsEnabled = true;
};
class ServerProcessCoordinator {
public:
  explicit ServerProcessCoordinator(client::Minecraft* minecraft);
  ~ServerProcessCoordinator();
  [[nodiscard]] static bool canHostWorld(const World* world);
  [[nodiscard]] bool canStartServer() const;
  [[nodiscard]] bool isStarting() const noexcept;
  [[nodiscard]] bool isAwaitingLoopback() const noexcept;
  [[nodiscard]] bool isActive() const noexcept;
  [[nodiscard]] bool isHostedWorld(const World* world) const noexcept;
  [[nodiscard]] std::uint16_t port() const noexcept;
  [[nodiscard]] const ServerConnectionInfo& connectionInfo() const noexcept;
  [[nodiscard]] const std::string& lastError() const noexcept;
  bool start(const ServerProcessSettings& settings, std::string& errorOut);
  bool pollStart(std::string& errorOut);
  void tick();
  void onConnectCanceledOrFailed(const std::string& error = {});
  void afterWorldChange(World* world);
  void shutdown();

private:
  enum class State { Inactive,
                     Starting,
                     AwaitingLoopback,
                     Active,
                     Stopping };
  bool fail(const std::string& error, std::string& errorOut);
  bool captureWorld(World& world, std::string& errorOut);
  bool launch(std::string& errorOut);
  bool processRunning() const;
  bool serverAcceptingConnections() const;
  void requestStop(bool restoreLocalWorld);
  void finishStop();
  void closeProcessHandles();
  client::Minecraft* minecraft_ = nullptr;
  State state_ = State::Inactive;
  ServerProcessSettings settings_;
  std::filesystem::path storageRoot_;
  std::string worldName_;
  std::uint64_t worldSeed_ = 0;
  World* remoteWorld_ = nullptr;
  ServerConnectionInfo connectionInfo_;
  std::string lastError_;
  bool restoreLocalWorld_ = false;
#ifdef _WIN32
  void* process_ = nullptr;
  void* processThread_ = nullptr;
  void* stdinWrite_ = nullptr;
#endif
};
} // namespace net::minecraft::client::host
