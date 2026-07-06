#include "net/minecraft/server/network/ServerLoginNetworkHandler.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/util/http/HttpClient.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/ChatPackets.hpp"
#include "net/minecraft/network/packet/WorldPackets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/server/network/ConnectionListener.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <random>
#include <sstream>
namespace net::minecraft::server::network {
namespace http = net::minecraft::util::http;
ServerLoginNetworkHandler::ServerLoginNetworkHandler(MinecraftServer* server, ConnectionListener* listener,
                                                     SOCKET socket, std::string connectionName, bool onlineMode)
    : server_(server), listener_(listener), onlineMode_(onlineMode) {
  Connection::configureAcceptedSocket(socket);
  connection_ = std::make_unique<Connection>(socket, connectionName, *this);
}
ServerLoginNetworkHandler::~ServerLoginNetworkHandler() {
  if(verifyThread_.joinable()) {
    verifyThread_.join();
  }
}
Connection* ServerLoginNetworkHandler::connection() const noexcept {
  return connection_.get();
}
void ServerLoginNetworkHandler::tick() {
  {
    std::lock_guard lock(verifyMutex_);
    if(deferredLoginPacket_.has_value()) {
      const LoginHelloPacket packet = *deferredLoginPacket_;
      deferredLoginPacket_.reset();
      accept(packet);
      return;
    }
  }
  if(loginTicks_++ == 600) {
    disconnect("Took too long to log in");
    return;
  }
  if(connection_ != nullptr) {
    connection_->tick();
  }
}
void ServerLoginNetworkHandler::disconnect(const std::string& reason) {
  try {
    std::cout << "Disconnecting " << getConnectionInfo() << ": " << reason << std::endl;
    if(connection_ != nullptr) {
      DisconnectPacket packet;
      packet.reason = reason;
      connection_->sendPacket(std::make_unique<DisconnectPacket>(packet));
      connection_->disconnect();
    }
    closed = true;
  } catch(const std::exception& error) {
    std::cerr << error.what() << std::endl;
  }
}
void ServerLoginNetworkHandler::onHandshake(const HandshakePacket& /*packet*/) {
  if(connection_ == nullptr) {
    return;
  }
  if(onlineMode_) {
    std::random_device device;
    std::mt19937_64 generator(device());
    std::uniform_int_distribution<std::uint64_t> distribution;
    std::ostringstream stream;
    stream << std::hex << distribution(generator);
    serverId_ = stream.str();
    connection_->sendPacket<HandshakePacket>(serverId_);
  } else {
    connection_->sendPacket<HandshakePacket>("-");
  }
}
void ServerLoginNetworkHandler::onHello(const LoginHelloPacket& packet) {
  username_ = packet.username;
  if(packet.protocolVersion != 14) {
    if(packet.protocolVersion > 14) {
      disconnect("Outdated server!");
    } else {
      disconnect("Outdated client!");
    }
    return;
  }
  if(!onlineMode_) {
    accept(packet);
    return;
  }
  verifyUsernameOnline(packet);
}
void ServerLoginNetworkHandler::verifyUsernameOnline(const LoginHelloPacket& packet) {
  if(verifyThread_.joinable()) {
    verifyThread_.join();
  }
  ServerLoginNetworkHandler* self = this;
  verifyThread_ = std::thread([self, packet]() {
    try {
      const std::string serverId = self->serverId_;
      std::ostringstream url;
      url << "http://www.minecraft.net/game/checkserver.jsp"
          << "?user=" << urlEncodeComponent(packet.username) << "&serverId=" << urlEncodeComponent(serverId);
      http::HttpRequest request;
      request.url = url.str();
      request.useBetacraftProxy = false;
      request.maxResponseBytes = 64U * 1024U;
      const http::HttpResponse response = http::httpRequest(request);
      const std::string body = response.bodyAsString();
      const std::size_t lineEnd = body.find_first_of("\r\n");
      const std::string firstLine = lineEnd == std::string::npos ? body : body.substr(0, lineEnd);
      if(firstLine == "YES") {
        std::lock_guard lock(self->verifyMutex_);
        self->deferredLoginPacket_.emplace(packet);
      } else {
        self->disconnect("Failed to verify username!");
      }
    } catch(const std::exception& error) {
      self->disconnect("Failed to verify username! [internal error " + std::string(error.what()) + "]");
      std::cerr << error.what() << std::endl;
    }
  });
}
void ServerLoginNetworkHandler::accept(const LoginHelloPacket& packet) {
  if(server_ == nullptr || listener_ == nullptr || connection_ == nullptr) {
    closed = true;
    return;
  }
  ::net::minecraft::entity::player::ServerPlayerEntity* player = server_->playerManager.connectPlayer(this, packet.username);
  if(player != nullptr) {
    server_->playerManager.loadPlayerData(player);
    player->setWorld(server_->getWorld(player->dimensionId));
    ServerWorld* serverWorld = server_->getWorld(player->dimensionId);
    std::cout << getConnectionInfo() << " logged in with entity id " << player->id << " at (" << player->x << ", "
              << player->y << ", " << player->z << ")" << std::endl;
    if(serverWorld != nullptr) {
      const Vec3i spawnPos = serverWorld->getSpawnPos();
      auto playHandler = std::make_unique<ServerPlayNetworkHandler>(server_, connection_.get(), player);
      const std::int8_t dimensionRawId =
          serverWorld->dimension != nullptr ? static_cast<std::int8_t>(serverWorld->dimension->id) : 0;
      LoginHelloPacket loginResponse;
      loginResponse.username.clear();
      loginResponse.protocolVersion = player->id;
      loginResponse.worldSeed = serverWorld->getSeed();
      loginResponse.dimensionId = dimensionRawId;
      playHandler->sendPacket(loginResponse);
      PlayerSpawnPositionS2CPacket spawnPacket;
      spawnPacket.x = spawnPos.x;
      spawnPacket.y = spawnPos.y;
      spawnPacket.z = spawnPos.z;
      playHandler->sendPacket(spawnPacket);
      server_->playerManager.sendWorldInfo(player, serverWorld);
      ChatMessagePacket joinMessage;
      joinMessage.chatMessage = "\u00a7e" + player->name + " joined the game.";
      server_->playerManager.sendToAll(joinMessage);
      server_->playerManager.addPlayer(player);
      playHandler->teleport(player->x, player->y, player->z, player->yaw, player->pitch);
      listener_->addConnection(std::move(playHandler), std::move(connection_));
      WorldTimeUpdateS2CPacket timePacket;
      timePacket.time = static_cast<std::int64_t>(serverWorld->getTime());
      if(player->networkHandler != nullptr) {
        player->networkHandler->sendPacket(timePacket);
      }
      player->initScreenHandler();
    }
  }
  closed = true;
}
void ServerLoginNetworkHandler::handle(const Packet& /*packet*/) {
  disconnect("Protocol error");
}
void ServerLoginNetworkHandler::onDisconnected(const std::string& /*reason*/, const std::vector<std::string>& /*args*/) {
  std::cout << getConnectionInfo() << " lost connection" << std::endl;
  closed = true;
}
std::string ServerLoginNetworkHandler::getConnectionInfo() const {
  if(!username_.empty()) {
    if(connection_ != nullptr) {
      return username_ + " [" + connection_->getAddress() + "]";
    }
    return username_;
  }
  if(connection_ != nullptr) {
    return connection_->getAddress();
  }
  return "unknown";
}
std::string ServerLoginNetworkHandler::urlEncodeComponent(const std::string& value) {
  static const char hex[] = "0123456789ABCDEF";
  std::string encoded;
  encoded.reserve(value.size());
  for(const unsigned char ch : value) {
    if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' ||
       ch == '_' || ch == '.' || ch == '~') {
      encoded += static_cast<char>(ch);
    } else {
      encoded += '%';
      encoded += hex[ch >> 4];
      encoded += hex[ch & 0x0F];
    }
  }
  return encoded;
}
} // namespace net::minecraft::server::network
