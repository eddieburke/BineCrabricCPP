#pragma once
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/server/ServerProperties.hpp"
#include "net/minecraft/server/command/CommandOutput.hpp"
#include "net/minecraft/util/Tickable.hpp"
#include "net/minecraft/server/entity/EntityTracker.hpp"
#include "net/minecraft/server/host/ServerLaunchConfig.hpp"
#include "net/minecraft/server/world/ServerWorldEventListener.hpp"
#include "net/minecraft/server/command/Command.hpp"
#include "net/minecraft/server/command/ServerCommandHandler.hpp"
#include <condition_variable>
#include <array>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
namespace net::minecraft {
class ServerWorld;
class RegionWorldStorage;
namespace server {
namespace network {
class ConnectionListener;
}
class MinecraftServer : public command::CommandOutput {
public:
  static std::unordered_map<std::string, int> capturedThread;
  static std::mutex capturedThreadMutex;
  MinecraftServer();
  explicit MinecraftServer(const host::ServerLaunchConfig& config);
  ~MinecraftServer();
  MinecraftServer(const MinecraftServer&) = delete;
  MinecraftServer& operator=(const MinecraftServer&) = delete;
  void run();
  void stop();
  bool startAsync();
  void stopAndJoin();
  void tick();
  void queueCommands(const std::string& string, command::CommandOutput& output);
  void runPendingCommands();
  void addTickable(util::Tickable* tickable);
  [[nodiscard]] std::uint16_t boundPort() const;
  [[nodiscard]] const std::string& lastError() const noexcept {
    return lastError_;
  }
  std::unique_ptr<network::ConnectionListener> connections;
  std::unique_ptr<ServerProperties> properties;
  ServerWorld* worlds[2] = {nullptr, nullptr};
  PlayerManager playerManager;
  std::array<EntityTracker, 2> entityTrackers;
  bool onlineMode = true;
  bool spawnAnimals = true;
  bool pvpEnabled = true;
  bool flightEnabled = false;
  bool allowNether = true;
  bool running = true;
  bool stopped = false;
  int ticks = 0;
  std::string progressMessage;
  int progress = 0;
  [[nodiscard]] bool isOperator(const std::string& name) const {
    return playerManager.isOperator(name);
  }
  [[nodiscard]] std::filesystem::path getFile(const std::string& path) const;
  void sendMessage(const std::string& message) override;
  void warn(const std::string& message);
  [[nodiscard]] std::string getName() override;
  ServerWorld* getWorld(int dimensionId);
  EntityTracker& getEntityTracker(int dimensionId);

private:
  bool init();
  void loadWorld(const std::filesystem::path& storageRoot, const std::string& worldDir, std::uint64_t seed);
  void shutdown();
  void saveWorlds();
  void logProgress(const std::string& progressType, int progressValue);
  void clearProgress();
  void notifyStartResult(bool success, std::string error = {});
  std::array<std::unique_ptr<ServerWorld>, 2> ownedWorlds_{};
  std::unique_ptr<RegionWorldStorage> worldStorage_;
  std::array<std::unique_ptr<world::ServerWorldEventListener>, 2> worldEventListeners_{};
  std::unique_ptr<command::ServerCommandHandler> commandHandler_;
  std::vector<util::Tickable*> tickables_;
  struct PendingCommand {
    std::string commandAndArgs;
    command::CommandOutput* output = nullptr;
  };
  std::deque<PendingCommand> pendingCommands_;
  std::mutex pendingCommandsMutex_;
  std::mutex startStateMutex_;
  std::condition_variable startStateCv_;
  std::jthread commandThread_;
  std::thread runThread_;
  std::optional<host::ServerLaunchConfig> launchConfig_;
  std::string lastError_;
  bool startResultReady_ = false;
  bool startSucceeded_ = false;
};
} // namespace server
} // namespace net::minecraft
