#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "net/minecraft/client/auth/LegacySessionAuth.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/util/concurrent/Channel.hpp"

namespace net::minecraft {
class Connection;
class Packet;
} // namespace net::minecraft

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::multiplayer {

class ClientLoginNetworkHandler : public NetworkHandler {
 public:
  ClientLoginNetworkHandler(client::Minecraft* minecraft);
  ~ClientLoginNetworkHandler();

  void tick() override;

  void bindConnection(Connection* connection) noexcept override {
    connection_ = connection;
  }
  [[nodiscard]] Connection* connection() const noexcept {
    return connection_;
  }

  void sendPacket(std::unique_ptr<Packet> packet) {
    if(disconnected || connection_ == nullptr || !connection_->isOpen()) {
      return;
    }
    connection_->sendPacket(std::move(packet));
  }

  template <typename PacketT>
  void sendPacket(const PacketT& packet) {
    if(disconnected || connection_ == nullptr || !connection_->isOpen()) {
      return;
    }
    connection_->sendPacket<PacketT>(packet);
  }

  void disconnect(const std::string& reason = "Disconnected") override;

  enum class PendingModDownloadState { Idle, Running, Succeeded, Failed };

  bool startPendingModDownload();
  PendingModDownloadState pollPendingModDownload(std::string& status, std::string& error);
  void continuePendingLogin();
  void cancelPendingModPrompt();

  [[nodiscard]] const std::vector<std::string>& pendingMissingMods() const noexcept {
    return pendingMissingMods_;
  }

  [[nodiscard]] bool isServerSide() const override {
    return false;
  }

  void onDisconnected(const std::string& reason, const std::vector<std::string>& objects) override;
  void onHandshake(const HandshakePacket& packet) override;
  void onHello(const LoginHelloPacket& packet) override;
  void onDisconnect(const DisconnectPacket& packet) override;
  void onKeepAlive(const KeepAlivePacket& packet) override;

  bool disconnected = false;
  std::string message;
  client::Minecraft* minecraft = nullptr;

 private:
  void processPendingJoinServer();
  void beginPendingLogin(const std::string& serverId);
  [[nodiscard]] std::vector<std::string> activeClientMods() const;

  Connection* connection_ = nullptr;

  struct JoinServerWork {
    std::atomic_bool cancelled{false};
    std::atomic_bool inFlight{false};
    ::net::minecraft::util::concurrent::Channel<auth::JoinServerResult> completed{1};
  };

  struct PendingModFile {
    std::filesystem::path temporary;
    std::filesystem::path destination;
  };

  struct PendingModEvent {
    enum class Kind { Progress, Complete, Failed };
    Kind kind = Kind::Progress;
    std::string text{};
    std::vector<PendingModFile> files{};
  };

  struct PendingModWork {
    std::atomic_bool cancelled{false};
    std::atomic_bool running{false};
    ::net::minecraft::util::concurrent::Channel<PendingModEvent> events{32};
  };

  static void downloadPendingMods(const std::shared_ptr<PendingModWork>& work,
                                  std::vector<std::string> missing,
                                  std::unordered_map<std::string, std::string> urls,
                                  std::filesystem::path temporaryDirectory,
                                  std::filesystem::path modsDirectory);

  std::shared_ptr<JoinServerWork> joinServerWork_ = std::make_shared<JoinServerWork>();
  std::shared_ptr<PendingModWork> pendingModWork_;

  enum class RemoteServerKind {
    JavaCompatible,
    NativeCppMods
  };

  RemoteServerKind remoteServerKind_ = RemoteServerKind::JavaCompatible;
  bool remoteLuaModsEnabled_ = false;
  bool waitingForModDownloadAcceptance_ = false;

  std::string pendingServerId_;
  std::vector<std::string> pendingMissingMods_;
  std::vector<std::string> pendingRequiredMods_;
  std::unordered_map<std::string, std::string> pendingDownloadUrls_;
};

} // namespace net::minecraft::client::multiplayer
