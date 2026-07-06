#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/block/BlockTypes.hpp"
#include "net/minecraft/client/gui/screen/LoadingDisplay.hpp"
#include "net/minecraft/network/packet/WorldPackets.hpp"
#include "net/minecraft/server/ServerLog.hpp"
#include "net/minecraft/server/command/Command.hpp"
#include "net/minecraft/server/network/ConnectionListener.hpp"
#include "net/minecraft/server/world/ReadOnlyServerWorld.hpp"
#include "net/minecraft/server/world/ServerWorldEventListener.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "net/minecraft/world/storage/RegionWorldStorage.hpp"
#include "net/minecraft/world/storage/RegionWorldStorageSource.hpp"
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace net::minecraft::server {
namespace {
using Clock = std::chrono::steady_clock;
class WorldConversionProgress final : public client::gui::screen::LoadingDisplay {
public:
  void progressStagePercentage(int percentage) override {
    const auto now = std::chrono::system_clock::now();
    if(now - lastLogTime_ >= std::chrono::seconds(1)) {
      lastLogTime_ = now;
      ServerLog::LOGGER.info("Converting... " + std::to_string(percentage) + "%");
    }
  }

private:
  std::chrono::system_clock::time_point lastLogTime_ = std::chrono::system_clock::now();
};
void warnIfLowMemory() {
#ifdef _WIN32
  MEMORYSTATUSEX status{};
  status.dwLength = sizeof(status);
  if(GlobalMemoryStatusEx(&status) && status.ullTotalPhys / (1024ULL * 1024ULL) < 512ULL) {
    ServerLog::LOGGER.log(LogLevel::Warning, "**** NOT ENOUGH RAM!");
    ServerLog::LOGGER.log(LogLevel::Warning,
                          "To start the server with more ram, launch it as "
                          "\"java -Xmx1024M -Xms1024M -jar minecraft_server.jar\"");
  }
#endif
}
} // namespace
std::unordered_map<std::string, int> MinecraftServer::capturedThread;
std::mutex MinecraftServer::capturedThreadMutex;
MinecraftServer::MinecraftServer()
    : playerManager(this),
      entityTrackers{EntityTracker(this, 0), EntityTracker(this, -1)} {
  backgroundDaemon_ = std::jthread([](const std::stop_token& stopToken) {
    while(!stopToken.stop_requested()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  });
}
MinecraftServer::MinecraftServer(const host::ServerLaunchConfig& config) : MinecraftServer() {
  launchConfig_ = config;
}
MinecraftServer::~MinecraftServer() {
  stopAndJoin();
  if(backgroundDaemon_.joinable()) {
    backgroundDaemon_.request_stop();
    backgroundDaemon_.join();
  }
  if(commandThread_.joinable()) {
    commandThread_.request_stop();
    commandThread_.join();
  }
  connections.reset();
  for(std::unique_ptr<ServerWorld>& world : ownedWorlds_) {
    world.reset();
  }
  worlds[0] = nullptr;
  worlds[1] = nullptr;
}
std::filesystem::path MinecraftServer::getFile(const std::string& path) const {
  return std::filesystem::path(path);
}
void MinecraftServer::sendMessage(const std::string& message) {
  ServerLog::LOGGER.info(message);
}
void MinecraftServer::warn(const std::string& message) {
  ServerLog::LOGGER.log(LogLevel::Warning, message);
}
std::string MinecraftServer::getName() {
  return "CONSOLE";
}
ServerWorld* MinecraftServer::getWorld(int dimensionId) {
  if(dimensionId == -1) {
    return worlds[1];
  }
  return worlds[0];
}
EntityTracker& MinecraftServer::getEntityTracker(int dimensionId) {
  if(dimensionId == -1) {
    return entityTrackers[1];
  }
  return entityTrackers[0];
}
void MinecraftServer::queueCommands(const std::string& string, command::CommandOutput& output) {
  std::lock_guard lock(pendingCommandsMutex_);
  pendingCommands_.push_back(PendingCommand{string, &output});
}
void MinecraftServer::runPendingCommands() {
  std::deque<PendingCommand> commands;
  {
    std::lock_guard lock(pendingCommandsMutex_);
    commands.swap(pendingCommands_);
  }
  if(commandHandler_ == nullptr) {
    return;
  }
  while(!commands.empty()) {
    PendingCommand pending = std::move(commands.front());
    commands.pop_front();
    if(pending.output == nullptr) {
      continue;
    }
    command::Command command(pending.commandAndArgs, *pending.output);
    commandHandler_->executeCommand(command);
  }
}
void MinecraftServer::addTickable(util::Tickable* tickable) {
  if(tickable != nullptr) {
    tickables_.push_back(tickable);
  }
}
bool MinecraftServer::startAsync() {
  stopAndJoin();
  {
    std::lock_guard lock(startStateMutex_);
    startResultReady_ = false;
    startSucceeded_ = false;
    lastError_.clear();
  }
  running = true;
  stopped = false;
  runThread_ = std::thread([this]() { run(); });
  std::unique_lock lock(startStateMutex_);
  startStateCv_.wait(lock, [this] { return startResultReady_; });
  const bool started = startSucceeded_;
  lock.unlock();
  if(!started && runThread_.joinable()) {
    runThread_.join();
  }
  return started;
}
void MinecraftServer::stopAndJoin() {
  stop();
  if(runThread_.joinable()) {
    runThread_.join();
  }
}
std::uint16_t MinecraftServer::boundPort() const {
  return connections != nullptr ? connections->boundPort() : 0;
}
bool MinecraftServer::init() {
  initializeBlocks();
  commandHandler_ = std::make_unique<command::ServerCommandHandler>(this);
  if(!launchConfig_.has_value() || launchConfig_->useConsoleThread) {
    commandThread_ = std::jthread([this](const std::stop_token& stopToken) {
      std::string line;
      std::streambuf* inputBuffer = std::cin.rdbuf();
      while(!stopToken.stop_requested() && running && !stopped) {
        if(inputBuffer == nullptr || inputBuffer->in_avail() <= 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          continue;
        }
        if(!std::getline(std::cin, line)) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          continue;
        }
        if(!line.empty()) {
          queueCommands(line, *this);
        }
      }
    });
  }
  ServerLog::init();
  ServerLog::LOGGER.info("Starting minecraft server version Beta 1.7.3");
  warnIfLowMemory();
  std::filesystem::path storageRoot = getFile(".");
  std::string worldName = "world";
  std::uint64_t seed = static_cast<std::uint64_t>(JavaRandom().nextLong());
  std::string bindAddress;
  int port = 25565;
  if(launchConfig_.has_value()) {
    const host::ServerLaunchConfig& config = *launchConfig_;
    properties = std::make_unique<ServerProperties>();
    properties->setProperty("server-ip", config.bindAddress);
    properties->setProperty("server-port", config.port);
    properties->setProperty("online-mode", config.onlineMode);
    properties->setProperty("spawn-animals", config.spawnAnimals);
    properties->setProperty("pvp", config.pvpEnabled);
    properties->setProperty("allow-flight", config.flightEnabled);
    properties->setProperty("allow-nether", config.allowNether);
    properties->setProperty("level-name", config.worldName);
    bindAddress = config.bindAddress;
    onlineMode = config.onlineMode;
    spawnAnimals = config.spawnAnimals;
    pvpEnabled = config.pvpEnabled;
    flightEnabled = config.flightEnabled;
    allowNether = config.allowNether;
    storageRoot = config.storageRoot;
    worldName = config.worldName;
    seed = config.worldSeed;
    port = config.port;
  } else {
    properties = std::make_unique<ServerProperties>(getFile("server.properties"));
    bindAddress = properties->getProperty("server-ip", std::string{});
    onlineMode = properties->getProperty("online-mode", true);
    spawnAnimals = properties->getProperty("spawn-animals", true);
    pvpEnabled = properties->getProperty("pvp", true);
    flightEnabled = properties->getProperty("allow-flight", false);
    allowNether = properties->getProperty("allow-nether", true);
    worldName = properties->getProperty("level-name", std::string("world"));
    std::string seedText = properties->getProperty("level-seed", std::string{});
    JavaRandom random;
    seed = static_cast<std::uint64_t>(random.nextLong());
    if(!seedText.empty()) {
      try {
        seed = static_cast<std::uint64_t>(std::stoll(seedText));
      } catch(...) {
        seed = static_cast<std::uint64_t>(std::hash<std::string>{}(seedText));
      }
    }
    port = properties->getProperty("server-port", 25565);
  }
  playerManager.configureFromProperties();
  ServerLog::LOGGER.info("Starting Minecraft server on " + (bindAddress.empty() ? "*" : bindAddress) + ":" +
                         std::to_string(port));
  try {
    connections = std::make_unique<network::ConnectionListener>(this, bindAddress, port, onlineMode);
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "**** FAILED TO BIND TO PORT!");
    ServerLog::LOGGER.log(LogLevel::Warning, std::string("The exception was: ") + exception.what());
    ServerLog::LOGGER.log(LogLevel::Warning, "Perhaps a server is already running on that port?");
    lastError_ = exception.what();
    return false;
  }
  if(!onlineMode) {
    ServerLog::LOGGER.log(LogLevel::Warning, "**** SERVER IS RUNNING IN OFFLINE/INSECURE MODE!");
    ServerLog::LOGGER.log(LogLevel::Warning, "The server will make no attempt to authenticate usernames. Beware.");
    ServerLog::LOGGER.log(LogLevel::Warning,
                          "While this makes the game possible to play without internet access, it also opens up the "
                          "ability for hackers to connect with any username they choose.");
    ServerLog::LOGGER.log(LogLevel::Warning,
                          "To change this, set \"online-mode\" to \"true\" in the server.settings file.");
  }
  const auto start = Clock::now();
  ServerLog::LOGGER.info("Preparing level \"" + worldName + "\"");
  loadWorld(storageRoot, worldName, seed);
  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
  ServerLog::LOGGER.info("Done (" + std::to_string(elapsed) + "ns)! For help, type \"help\" or \"?\"");
  return true;
}
void MinecraftServer::loadWorld(const std::filesystem::path& storageRoot, const std::string& worldDir,
                                std::uint64_t seed) {
  RegionWorldStorageSource storageSource(storageRoot);
  if(storageSource.needsConversion(worldDir)) {
    ServerLog::LOGGER.info("Converting map!");
    WorldConversionProgress conversionProgress;
    storageSource.convert(worldDir, &conversionProgress);
  }
  auto regionStorage = std::make_unique<RegionWorldStorage>(storageRoot, worldDir, true);
  worldStorage_ = std::move(regionStorage);
  RegionWorldStorage* storagePtr = worldStorage_.get();
  ownedWorlds_[0] = std::make_unique<ServerWorld>(this, storagePtr, worldDir, 0, seed);
  if(ownedWorlds_[0]->isNewWorld()) {
    ownedWorlds_[0]->initializeSpawnPoint();
  }
  ownedWorlds_[1] =
      std::make_unique<world::ReadOnlyServerWorld>(this, storagePtr, worldDir, -1, seed, ownedWorlds_[0].get());
  worlds[0] = ownedWorlds_[0].get();
  worlds[1] = ownedWorlds_[1].get();
  for(int i = 0; i < 2; ++i) {
    ServerWorld* world = worlds[i];
    if(world == nullptr) {
      continue;
    }
    worldEventListeners_[static_cast<std::size_t>(i)] = std::make_unique<world::ServerWorldEventListener>(this, world);
    world->addEventListener(worldEventListeners_[static_cast<std::size_t>(i)].get());
    const bool spawnMonsters = properties != nullptr ? properties->getProperty("spawn-monsters", true) : true;
    world->difficulty = spawnMonsters ? 1 : 0;
    world->allowSpawning(spawnMonsters, spawnAnimals);
  }
  playerManager.saveAllPlayers(worlds);
  constexpr int radius = 196;
  auto lastProgress = std::chrono::system_clock::now();
  for(int i = 0; i < 2; ++i) {
    if(i != 0 && properties != nullptr && !properties->getProperty("allow-nether", true)) {
      continue;
    }
    ServerWorld* world = worlds[i];
    if(world == nullptr) {
      continue;
    }
    ServerLog::LOGGER.info("Preparing start region for level " + std::to_string(i));
    const Vec3i spawnPos = world->getSpawnPos();
    for(int j = -radius; j <= radius && running; j += 16) {
      for(int k = -radius; k <= radius && running; k += 16) {
        const auto now = std::chrono::system_clock::now();
        if(now < lastProgress) {
          lastProgress = now;
        }
        if(now > lastProgress + std::chrono::seconds(1)) {
          const int total = (radius * 2 + 1) * (radius * 2 + 1);
          const int current = (j + radius) * (radius * 2 + 1) + (k + 1);
          logProgress("Preparing spawn area", current * 100 / total);
          lastProgress = now;
        }
        if(ChunkCache* cache = dynamic_cast<ChunkCache*>(world->getChunkSource()); cache != nullptr) {
          cache->loadChunk((spawnPos.x + j) >> 4, (spawnPos.z + k) >> 4);
        }
        while(world->doLightingUpdates() && running) {
        }
      }
    }
  }
  clearProgress();
}
void MinecraftServer::logProgress(const std::string& progressType, int progressValue) {
  progressMessage = progressType;
  progress = progressValue;
  ServerLog::LOGGER.info(progressType + ": " + std::to_string(progressValue) + "%");
}
void MinecraftServer::clearProgress() {
  progressMessage.clear();
  progress = 0;
}
void MinecraftServer::saveWorlds() {
  ServerLog::LOGGER.info("Saving chunks");
  for(int i = 0; i < 2; ++i) {
    ServerWorld* world = worlds[i];
    if(world == nullptr) {
      continue;
    }
    world->saveWithLoadingDisplay(true, nullptr);
    world->forceSave();
  }
}
void MinecraftServer::shutdown() {
  ServerLog::LOGGER.info("Stopping server");
  playerManager.savePlayers();
  if(worlds[0] != nullptr || worlds[1] != nullptr) {
    saveWorlds();
  }
}
void MinecraftServer::stop() {
  running = false;
  if(connections != nullptr) {
    connections->close();
  }
}
void MinecraftServer::run() {
  bool initialized = false;
  try {
    initialized = init();
    if(initialized) {
      notifyStartResult(true);
      auto lastTick = Clock::now();
      std::int64_t catchupMs = 0;
      while(running) {
        const auto now = Clock::now();
        auto deltaMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count();
        if(deltaMs > 2000) {
          ServerLog::LOGGER.log(LogLevel::Warning,
                                "Can't keep up! Did the system time change, or is the server overloaded?");
          deltaMs = 2000;
        }
        if(deltaMs < 0) {
          ServerLog::LOGGER.log(LogLevel::Warning, "Time ran backwards! Did the system time change?");
          deltaMs = 0;
        }
        catchupMs += deltaMs;
        lastTick = now;
        if(worlds[0] != nullptr && worlds[0]->canSkipNight()) {
          tick();
          catchupMs = 0;
        } else {
          while(catchupMs > 50) {
            catchupMs -= 50;
            tick();
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    } else {
      notifyStartResult(false, lastError_);
      stopped = true;
      running = false;
      return;
    }
  } catch(const std::exception& exception) {
    std::cerr << exception.what() << std::endl;
    ServerLog::LOGGER.log(LogLevel::Severe, "Unexpected exception", &exception);
    notifyStartResult(false, exception.what());
    lastError_ = exception.what();
  }
  try {
    if(initialized) {
      shutdown();
    }
    stopped = true;
  } catch(const std::exception& exception) {
    std::cerr << exception.what() << std::endl;
  }
}
void MinecraftServer::notifyStartResult(bool success, std::string error) {
  std::lock_guard lock(startStateMutex_);
  if(startResultReady_) {
    return;
  }
  startSucceeded_ = success;
  startResultReady_ = true;
  if(!error.empty()) {
    lastError_ = std::move(error);
  }
  startStateCv_.notify_all();
}
void MinecraftServer::tick() {
  std::vector<std::string> expiredThreads;
  {
    std::lock_guard lock(capturedThreadMutex);
    for(const auto& entry : capturedThread) {
      if(entry.second > 0) {
        capturedThread[entry.first] = entry.second - 1;
      } else {
        expiredThreads.push_back(entry.first);
      }
    }
    for(const std::string& key : expiredThreads) {
      capturedThread.erase(key);
    }
  }
  Box::resetCacheCount();
  Vec3d::resetCacheCount();
  ++ticks;
  for(int i = 0; i < 2; ++i) {
    if(i != 0 && !allowNether) {
      continue;
    }
    ServerWorld* world = worlds[i];
    if(world == nullptr || world->dimension == nullptr) {
      continue;
    }
    if(ticks % 20 == 0) {
      WorldTimeUpdateS2CPacket packet;
      packet.time = static_cast<std::int64_t>(world->getTime());
      playerManager.sendToDimension(packet, world->dimension->id);
    }
    world->tick();
    while(world->doLightingUpdates()) {
    }
    world->tickEntities();
  }
  if(connections != nullptr) {
    connections->tick();
  }
  playerManager.updateAllChunks();
  entityTrackers[0].tick();
  entityTrackers[1].tick();
  for(util::Tickable* tickable : tickables_) {
    if(tickable != nullptr) {
      tickable->tick();
    }
  }
  try {
    runPendingCommands();
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "Unexpected exception while parsing console command", &exception);
  }
}
} // namespace net::minecraft::server
