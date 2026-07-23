#include "net/minecraft/server/network/ServerLoginNetworkHandler.hpp"
#include <random>
#include <sstream>
#include <unordered_map>
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/HandshakeMetadata.hpp"
#include "net/minecraft/network/packet/ChatPackets.hpp"
#include "net/minecraft/network/packet/LuaModSyncPacket.hpp"
#include "net/minecraft/network/packet/WorldPackets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/server/ServerLog.hpp"
#include "net/minecraft/server/network/ConnectionListener.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/util/http/HttpClient.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
namespace net::minecraft::server::network {
namespace http = net::minecraft::util::http;
ServerLoginNetworkHandler::ServerLoginNetworkHandler(
    MinecraftServer* server, ConnectionListener* listener, SOCKET socket, std::string connectionName, bool onlineMode)
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
  server::ServerLog::LOGGER.info("Disconnecting " + getConnectionInfo() + ": " + reason);
  if(connection_ != nullptr) {
   DisconnectPacket packet;
   packet.reason = reason;
   connection_->sendPacket(std::make_unique<DisconnectPacket>(packet));
   connection_->disconnect();
  }
  closed = true;
 } catch(const std::exception&) {
 }
}
std::vector<std::string> ServerLoginNetworkHandler::requiredWorldMods() const {
 if(server_ == nullptr) {
  return {};
 }
 ServerWorld* world = server_->getWorld(0);
 if(world == nullptr || world->getDimensionData() == nullptr) {
  return {};
 }
 return mod::runtime::WorldRequiredMods::requiredForWorld(world->getDimensionData()->worldDirectory(), world);
}
namespace {
std::unordered_map<std::string, std::string> downloadUrlsForRequiredMods(const std::vector<std::string>& required) {
 std::unordered_map<std::string, std::string> urls;
 const std::vector<mod::runtime::ModPackage> packages = mod::runtime::host().packageMods();
 for(const std::string& modId : required) {
  const auto it = std::find_if(packages.begin(), packages.end(), [&modId](const mod::runtime::ModPackage& pkg) {
   return pkg.id == modId && !pkg.downloadUrl.empty();
  });
  if(it != packages.end()) {
   urls.emplace(modId, it->downloadUrl);
  }
 }
 return urls;
}
} // namespace
void ServerLoginNetworkHandler::onHandshake(const HandshakePacket& /*packet*/) {
 if(connection_ == nullptr) {
  return;
 }
 std::string reply = "-";
 if(onlineMode_) {
  std::random_device device;
  std::mt19937_64 generator(device());
  std::uniform_int_distribution<std::uint64_t> distribution;
  std::ostringstream stream;
  stream << std::hex << distribution(generator);
  serverId_ = stream.str();
  reply = serverId_;
 }
 const std::vector<std::string> required = requiredWorldMods();
 const bool luaModsAvailable = !mod::runtime::host().loadedMods().empty();
 if(luaModsAvailable) {
  reply = ::net::minecraft::network::appendHandshakeMetadata(
      reply, true, true, required, downloadUrlsForRequiredMods(required));
 }
 connection_->sendPacket<HandshakePacket>(reply);
}
void ServerLoginNetworkHandler::onLuaModSync(const LuaModSyncPacket& packet) {
 if(packet.kind != LuaModSyncKind::ClientModList) {
  return;
 }
 clientModsCsv_.assign(packet.payload.begin(), packet.payload.end());
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
 const std::vector<std::string> missing = mod::runtime::WorldRequiredMods::missingFrom(
     requiredWorldMods(), mod::runtime::WorldRequiredMods::splitCsv(clientModsCsv_));
 if(!missing.empty()) {
  disconnect(mod::runtime::WorldRequiredMods::requirementMessage(missing));
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
  }
 });
}
void ServerLoginNetworkHandler::accept(const LoginHelloPacket& packet) {
 if(server_ == nullptr || listener_ == nullptr || connection_ == nullptr) {
  closed = true;
  return;
 }
 ::net::minecraft::entity::player::ServerPlayerEntity* player =
     server_->playerManager.connectPlayer(this, packet.username);
 if(player == nullptr) {
  closed = true;
  return;
 }
 server_->playerManager.loadPlayerData(player);
 player->setWorld(server_->getWorld(player->dimensionId));
 ServerWorld* serverWorld = server_->getWorld(player->dimensionId);
 if(serverWorld == nullptr) {
  disconnect("Internal server error: world unavailable");
  closed = true;
  return;
 }
 {
  std::ostringstream loginMessage;
  loginMessage << getConnectionInfo() << " logged in with entity id " << player->id << " at (" << player->x
               << ", " << player->y << ", " << player->z << ")";
  server::ServerLog::LOGGER.info(loginMessage.str());
 }
 {
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
 closed = true;
}
void ServerLoginNetworkHandler::handle(const Packet& /*packet*/) {
 disconnect("Protocol error");
}
void ServerLoginNetworkHandler::onDisconnected(const std::string& /*reason*/,
                                               const std::vector<std::string>& /*args*/) {
 server::ServerLog::LOGGER.info(getConnectionInfo() + " lost connection");
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
