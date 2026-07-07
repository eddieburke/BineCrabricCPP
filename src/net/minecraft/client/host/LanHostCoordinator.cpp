#include "net/minecraft/client/host/LanHostCoordinator.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/core/WorldSession.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/host/ServerLaunchConfig.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
#include <utility>
namespace net::minecraft::client::host {
LanHostCoordinator::LanHostCoordinator(client::Minecraft* minecraft) : minecraft_(minecraft) {}
LanHostCoordinator::~LanHostCoordinator() {
  shutdown();
}
bool LanHostCoordinator::canHostWorld(const World* world) {
  if(world == nullptr || world->isRemote()) {
    return false;
  }
  const WorldStorage* storage = world->getDimensionData();
  return storage != nullptr && !storage->worldDirectory().empty() && !storage->worldName().empty();
}
bool LanHostCoordinator::canOpenLan() const {
  return minecraft_ != nullptr && canHostWorld(minecraft_->world);
}
bool LanHostCoordinator::isStartingServer() const noexcept {
  return state_ == State::StartingServer;
}
bool LanHostCoordinator::isAwaitingLoopback() const noexcept {
  return state_ == State::AwaitingLoopback;
}
bool LanHostCoordinator::isHosting() const noexcept {
  return state_ == State::Active;
}
bool LanHostCoordinator::isHostedWorld(const World* world) const noexcept {
  return state_ == State::Active && world != nullptr && world == hostedRemoteWorld_;
}
std::uint16_t LanHostCoordinator::requestedPort() const noexcept {
  return requestedPort_;
}
std::uint16_t LanHostCoordinator::boundPort() const noexcept {
  return boundPort_;
}
const LanConnectionInfo& LanHostCoordinator::connectionInfo() const noexcept {
  return connectionInfo_;
}
const std::string& LanHostCoordinator::lastError() const noexcept {
  return lastError_;
}
bool LanHostCoordinator::failBeginHosting(const std::string& error, std::string& errorOut) {
  errorOut = error;
  lastError_ = error;
  return false;
}
void LanHostCoordinator::clearServerStartResult() {
  std::lock_guard lock(serverStartMutex_);
  serverStartResult_ = {};
}
void LanHostCoordinator::beginServerTeardown() {
  if(server_ != nullptr) {
    server_->stop();
  }
  if(serverStartThread_.joinable()) {
    serverStartThread_.join();
  }
  clearServerStartResult();
  if(server_ == nullptr) {
    return;
  }
  auto done = std::make_shared<std::atomic<bool>>(false);
  std::thread worker([server = std::move(server_), done]() mutable {
    server->stopAndJoin();
    server.reset();
    done->store(true, std::memory_order_release);
  });
  teardowns_.push_back(PendingTeardown{std::move(worker), std::move(done)});
}
void LanHostCoordinator::startTeardown(bool restoreLocalWorld) {
  beginServerTeardown();
  restoreLocalWorldAfterTeardown_ = restoreLocalWorld && minecraft_ != nullptr;
  if(restoreLocalWorld) {
    state_ = State::TearingDown;
    return;
  }
  resetHostFields(false);
}
void LanHostCoordinator::failServerStart(std::string error) {
  if(error.empty()) {
    error = "Failed to start integrated server";
  }
  lastError_ = std::move(error);
  startTeardown(true);
}
void LanHostCoordinator::reapFinishedTeardowns() {
  auto it = teardowns_.begin();
  while(it != teardowns_.end()) {
    if(it->done->load(std::memory_order_acquire)) {
      if(it->thread.joinable()) {
        it->thread.join();
      }
      it = teardowns_.erase(it);
    } else {
      ++it;
    }
  }
}
void LanHostCoordinator::joinAllTeardowns() {
  for(PendingTeardown& teardown : teardowns_) {
    if(teardown.thread.joinable()) {
      teardown.thread.join();
    }
  }
  teardowns_.clear();
}
void LanHostCoordinator::finishServerStart(bool ok, std::uint16_t boundPort, std::string error) {
  if(!ok) {
    failServerStart(std::move(error));
    return;
  }
  boundPort_ = boundPort;
  if(boundPort_ == 0) {
    failServerStart("Integrated server did not report a bound port.");
    return;
  }
  connectionInfo_ = LanAddressResolver::resolve(boundPort_);
  state_ = State::AwaitingLoopback;
}
bool LanHostCoordinator::beginHosting(std::uint16_t port, std::string& errorOut) {
  errorOut.clear();
  lastError_.clear();
  if(minecraft_ == nullptr) {
    return failBeginHosting("Minecraft client is unavailable.", errorOut);
  }
  if(state_ != State::Inactive) {
    return failBeginHosting("A LAN host is already running.", errorOut);
  }
  if(port == 0) {
    return failBeginHosting("Port must be between 1 and 65535.", errorOut);
  }
  World* world = minecraft_->world;
  if(!canHostWorld(world)) {
    return failBeginHosting("Open to LAN is only available from a local saved world.", errorOut);
  }
  WorldStorage* storage = world->getDimensionData();
  if(storage == nullptr) {
    return failBeginHosting("Current world storage is unavailable.", errorOut);
  }
  storageRoot_ = storage->worldDirectory().parent_path();
  worldName_ = storage->worldName();
  worldSeed_ = world->getSeed();
  if(storageRoot_.empty() || worldName_.empty()) {
    return failBeginHosting("Could not resolve the current world's save directory.", errorOut);
  }
  if(!minecraft_->worldSession().parkLocalWorldForRemoteHandoff(*minecraft_)) {
    return failBeginHosting("Could not release the local world for LAN hosting.", errorOut);
  }
  server::host::ServerLaunchConfig config;
  config.storageRoot = storageRoot_;
  config.worldName = worldName_;
  config.worldSeed = worldSeed_;
  config.bindAddress.clear();
  config.port = static_cast<int>(port);
  config.onlineMode = false;
  config.useConsoleThread = false;
  config.useGui = false;
  requestedPort_ = port;
  server_ = std::make_unique<server::MinecraftServer>(config);
  clearServerStartResult();
  if(serverStartThread_.joinable()) {
    serverStartThread_.join();
  }
  serverStartThread_ = std::thread([this]() {
    ServerStartResult result;
    result.ok = server_ != nullptr && server_->startAsync();
    if(!result.ok && server_ != nullptr) {
      result.error = server_->lastError();
    }
    if(result.ok && server_ != nullptr) {
      result.boundPort = server_->boundPort();
    }
    result.done = true;
    std::lock_guard lock(serverStartMutex_);
    serverStartResult_ = std::move(result);
  });
  state_ = State::StartingServer;
  return true;
}
bool LanHostCoordinator::tickHosting(std::string& errorOut) {
  errorOut.clear();
  if(state_ != State::StartingServer) {
    return false;
  }
  ServerStartResult result;
  {
    std::lock_guard lock(serverStartMutex_);
    result = serverStartResult_;
  }
  if(!result.done) {
    return false;
  }
  if(serverStartThread_.joinable()) {
    serverStartThread_.join();
  }
  finishServerStart(result.ok, result.boundPort, std::move(result.error));
  if(state_ == State::Inactive) {
    errorOut = lastError_;
    return true;
  }
  return true;
}
void LanHostCoordinator::resetHostFields(bool clearError) {
  hostedRemoteWorld_ = nullptr;
  storageRoot_.clear();
  worldName_.clear();
  worldSeed_ = 0;
  requestedPort_ = 0;
  boundPort_ = 0;
  connectionInfo_ = {};
  state_ = State::Inactive;
  if(clearError) {
    lastError_.clear();
  }
}
void LanHostCoordinator::finalizeTeardown() {
  const bool restore = restoreLocalWorldAfterTeardown_;
  restoreLocalWorldAfterTeardown_ = false;
  if(restore && minecraft_ != nullptr) {
    (void)minecraft_->worldSession().restoreParkedLocalWorld(*minecraft_);
  }
  resetHostFields(false);
}
void LanHostCoordinator::tickBackground() {
  reapFinishedTeardowns();
  if(state_ != State::TearingDown) {
    return;
  }
  if(!teardowns_.empty()) {
    return;
  }
  finalizeTeardown();
}
void LanHostCoordinator::onConnectCanceledOrFailed(const std::string& error) {
  if(state_ != State::StartingServer && state_ != State::AwaitingLoopback) {
    return;
  }
  if(!error.empty()) {
    lastError_ = error;
  }
  startTeardown(true);
}
void LanHostCoordinator::afterWorldChange(World* world) {
  if(state_ == State::AwaitingLoopback) {
    if(world == nullptr) {
      onConnectCanceledOrFailed();
      return;
    }
    if(world->isRemote()) {
      hostedRemoteWorld_ = world;
      state_ = State::Active;
      if(minecraft_ != nullptr) {
        minecraft_->worldSession().commitRemoteHandoff();
      }
    }
    return;
  }
  if(state_ == State::Active && world != hostedRemoteWorld_) {
    startTeardown(false);
  }
}
void LanHostCoordinator::shutdown() {
  if(server_ != nullptr) {
    beginServerTeardown();
  }
  if(serverStartThread_.joinable()) {
    serverStartThread_.join();
  }
  joinAllTeardowns();
  restoreLocalWorldAfterTeardown_ = false;
  resetHostFields(false);
}
} // namespace net::minecraft::client::host
