#include "net/minecraft/server/network/ConnectionListener.hpp"
#include <stdexcept>
#include <utility>
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/network/ServerLoginNetworkHandler.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
namespace net::minecraft::server::network {
namespace {
[[nodiscard]] std::string hostFromAddress(const std::string& address) {
  const std::size_t colon = address.rfind(':');
  if(colon == std::string::npos) {
    return address;
  }
  return address.substr(0, colon);
}
[[nodiscard]] bool isLoopbackHost(const std::string& host) {
  return host == "127.0.0.1" || host == "::1" || host == "0:0:0:0:0:0:0:1";
}
} // namespace
ConnectionListener::ConnectionListener(MinecraftServer* server,
                                       const std::string& bindAddress,
                                       int port,
                                       bool onlineMode)
    : server_(server), onlineMode_(onlineMode) {
  socket_.bindAndListen(bindAddress, static_cast<std::uint16_t>(port));
  open_.store(true, std::memory_order_release);
  thread_ = std::thread([this]() { listenLoop(); });
}
ConnectionListener::~ConnectionListener() {
  close();
}
void ConnectionListener::stopAccepting() {
  std::call_once(acceptStopFlag_, [this]() {
    open_.store(false, std::memory_order_release);
    socket_.close();
    if(thread_.joinable()) {
      thread_.join();
    }
  });
}
void ConnectionListener::close() {
  stopAccepting();
  std::lock_guard lock(mutex_);
  pendingConnections_.clear();
  playConnections_.clear();
}
void ConnectionListener::listenLoop() {
  while(open_.load(std::memory_order_acquire)) {
    std::string remoteAddress;
    const SOCKET clientSocket = socket_.accept(remoteAddress);
    if(clientSocket == INVALID_SOCKET) {
      if(!open_.load(std::memory_order_acquire)) {
        break;
      }
      continue;
    }
    const std::string host = hostFromAddress(remoteAddress);
    const auto now = std::chrono::steady_clock::now();
    {
      std::lock_guard lock(mutex_);
      const auto found = recentConnectionsByHost_.find(host);
      if(!isLoopbackHost(host) && found != recentConnectionsByHost_.end() &&
         now - found->second < std::chrono::milliseconds(5000)) {
        recentConnectionsByHost_[host] = now;
        ::closesocket(clientSocket);
        continue;
      }
      recentConnectionsByHost_[host] = now;
    }
    std::string connectionName = "Connection #" + std::to_string(connectionCounter_++);
    try {
      auto loginHandler = std::make_unique<ServerLoginNetworkHandler>(
          server_, this, clientSocket, std::move(connectionName), onlineMode_);
      addPendingConnection(std::move(loginHandler));
    } catch(const std::exception&) {
      ::closesocket(clientSocket);
    }
  }
}
void ConnectionListener::addPendingConnection(std::unique_ptr<ServerLoginNetworkHandler> connection) {
  if(connection == nullptr) {
    throw std::invalid_argument("Got null pending connection");
  }
  std::lock_guard lock(mutex_);
  pendingConnections_.push_back(std::move(connection));
}
void ConnectionListener::addConnection(std::unique_ptr<ServerPlayNetworkHandler> handler,
                                       std::unique_ptr<Connection> connection) {
  std::lock_guard lock(mutex_);
  ActivePlayConnection active;
  active.connection = std::move(connection);
  active.handler = std::move(handler);
  playConnections_.push_back(std::move(active));
}
std::uint16_t ConnectionListener::boundPort() const {
  return socket_.boundPort();
}
void ConnectionListener::tick() {
  std::vector<std::unique_ptr<ServerLoginNetworkHandler>> pendingSnapshot;
  std::vector<ActivePlayConnection> playSnapshot;
  {
    std::lock_guard lock(mutex_);
    pendingSnapshot = std::move(pendingConnections_);
    pendingConnections_.clear();
    playSnapshot = std::move(playConnections_);
    playConnections_.clear();
  }
  for(std::size_t index = 0; index < pendingSnapshot.size();) {
    ServerLoginNetworkHandler& handler = *pendingSnapshot[index];
    try {
      handler.tick();
    } catch(const std::exception&) {
      handler.disconnect("Internal server error");
    }
    if(handler.closed) {
      pendingSnapshot.erase(pendingSnapshot.begin() + static_cast<std::ptrdiff_t>(index));
      continue;
    }
    if(Connection* connection = handler.connection()) {
      connection->interrupt();
    }
    ++index;
  }
  for(std::size_t index = 0; index < playSnapshot.size();) {
    ActivePlayConnection& active = playSnapshot[index];
    try {
      if(active.handler != nullptr) {
        active.handler->tick();
      }
    } catch(const std::exception&) {
      if(active.handler != nullptr) {
        active.handler->disconnect("Internal server error");
      }
    }
    const bool disconnected = active.handler == nullptr || active.handler->disconnected ||
                              active.connection == nullptr || !active.connection->isOpen();
    if(disconnected) {
      playSnapshot.erase(playSnapshot.begin() + static_cast<std::ptrdiff_t>(index));
      continue;
    }
    active.connection->interrupt();
    ++index;
  }
  {
    std::lock_guard lock(mutex_);
    for(std::unique_ptr<ServerLoginNetworkHandler>& handler : pendingSnapshot) {
      pendingConnections_.push_back(std::move(handler));
    }
    for(ActivePlayConnection& active : playSnapshot) {
      playConnections_.push_back(std::move(active));
    }
  }
}
} // namespace net::minecraft::server::network
