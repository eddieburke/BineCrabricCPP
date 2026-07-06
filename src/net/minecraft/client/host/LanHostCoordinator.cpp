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
void LanHostCoordinator::stopServer() {
  if(server_ != nullptr) {
    server_->stop();
    server_->stopAndJoin();
    server_.reset();
  }
  if(serverStartThread_.joinable()) {
    serverStartThread_.join();
  }
  {
    std::lock_guard lock(serverStartMutex_);
    serverStartResult_ = {};
  }
}
void LanHostCoordinator::finishServerStart(bool ok, std::uint16_t boundPort, std::string error) {
  if(!ok) {
    if(error.empty()) {
      error = "Failed to start integrated server";
    }
    lastError_ = error;
    stopServer();
    if(minecraft_ != nullptr) {
      (void)minecraft_->worldSession().restoreParkedLocalWorld(*minecraft_);
    }
    clearState(false);
    return;
  }
  boundPort_ = boundPort;
  if(boundPort_ == 0) {
    lastError_ = "Integrated server did not report a bound port.";
    stopServer();
    if(minecraft_ != nullptr) {
      (void)minecraft_->worldSession().restoreParkedLocalWorld(*minecraft_);
    }
    clearState(false);
    return;
  }
  connectionInfo_ = LanAddressResolver::resolve(boundPort_);
  state_ = State::AwaitingLoopback;
}
bool LanHostCoordinator::beginHosting(std::uint16_t port, std::string& errorOut) {
  errorOut.clear();
  lastError_.clear();
  if(minecraft_ == nullptr) {
    errorOut = "Minecraft client is unavailable.";
    lastError_ = errorOut;
    return false;
  }
  if(state_ != State::Inactive) {
    errorOut = "A LAN host is already running.";
    lastError_ = errorOut;
    return false;
  }
  if(port == 0) {
    errorOut = "Port must be between 1 and 65535.";
    lastError_ = errorOut;
    return false;
  }
  World* world = minecraft_->world;
  if(!canHostWorld(world)) {
    errorOut = "Open to LAN is only available from a local saved world.";
    lastError_ = errorOut;
    return false;
  }
  WorldStorage* storage = world->getDimensionData();
  if(storage == nullptr) {
    errorOut = "Current world storage is unavailable.";
    lastError_ = errorOut;
    return false;
  }
  storageRoot_ = storage->worldDirectory().parent_path();
  worldName_ = storage->worldName();
  worldSeed_ = world->getSeed();
  if(storageRoot_.empty() || worldName_.empty()) {
    errorOut = "Could not resolve the current world's save directory.";
    lastError_ = errorOut;
    return false;
  }
  if(!minecraft_->worldSession().parkLocalWorldForRemoteHandoff(*minecraft_)) {
    errorOut = "Could not release the local world for LAN hosting.";
    lastError_ = errorOut;
    return false;
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
  {
    std::lock_guard lock(serverStartMutex_);
    serverStartResult_ = {};
  }
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
void LanHostCoordinator::clearState(bool clearError) {
  stopServer();
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
void LanHostCoordinator::onConnectCanceledOrFailed(const std::string& error) {
  if(state_ == State::StartingServer) {
    if(!error.empty()) {
      lastError_ = error;
    }
    stopServer();
    if(minecraft_ != nullptr) {
      (void)minecraft_->worldSession().restoreParkedLocalWorld(*minecraft_);
    }
    clearState(false);
    return;
  }
  if(state_ != State::AwaitingLoopback) {
    return;
  }
  if(!error.empty()) {
    lastError_ = error;
  }
  stopServer();
  if(minecraft_ != nullptr) {
    (void)minecraft_->worldSession().restoreParkedLocalWorld(*minecraft_);
  }
  clearState(false);
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
    stopServer();
    clearState(false);
  }
}
void LanHostCoordinator::shutdown() {
  if(state_ == State::StartingServer || state_ == State::AwaitingLoopback) {
    onConnectCanceledOrFailed();
    return;
  }
  stopServer();
  clearState(false);
}
} // namespace net::minecraft::client::host
