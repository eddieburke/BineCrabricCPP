#pragma once
#include "net/minecraft/client/auth/LegacySessionAuth.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
namespace net::minecraft {
class Connection;
class Packet;
class SimpleInventory;
} // namespace net::minecraft
namespace net::minecraft::entity::player {
class ClientPlayerEntity;
}
namespace net::minecraft::block::entity {
class DispenserBlockEntity;
class FurnaceBlockEntity;
} // namespace net::minecraft::block::entity
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::entity {
class Entity;
}
namespace net::minecraft::client::multiplayer {
[[nodiscard]] std::unique_ptr<Packet> makePlayerMoveResponsePacket(
    const PlayerMovePacket& packet, const entity::player::ClientPlayerEntity& player);
class ClientNetworkHandler : public NetworkHandler {
public:
  ClientNetworkHandler(client::Minecraft* minecraft);
  ~ClientNetworkHandler();
  void tick();
  void bindConnection(Connection* connection) noexcept {
    connection_ = connection;
  }
  [[nodiscard]] Connection* connection() const noexcept {
    return connection_;
  }
  void onHello(std::uint64_t worldSeed, int dimensionId, int playerEntityId);
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
  template <typename PacketT>
  void sendPacketAndDisconnect(const PacketT& packet) {
    sendPacket(packet);
    disconnected = true;
  }
  void disconnect(const std::string& reason = "Disconnected");
  bool downloadPendingMods(std::string& error);
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
  void onChatMessage(const ChatMessagePacket& packet) override;
  void onDisconnect(const DisconnectPacket& packet) override;
  void onKeepAlive(const KeepAlivePacket& packet) override;
  void onEntity(const EntityS2CPacket& packet) override;
  void onEntitySpawn(const EntitySpawnS2CPacket& packet) override;
  void onPlayerMove(const PlayerMovePacket& packet) override;
  void onPlayerRespawn(const PlayerRespawnPacket& packet) override;
  void onPlayerSpawn(const PlayerSpawnS2CPacket& packet) override;
  void onPlayerSpawnPosition(const PlayerSpawnPositionS2CPacket& packet) override;
  void onWorldTimeUpdate(const WorldTimeUpdateS2CPacket& packet) override;
  void onHealthUpdate(const HealthUpdateS2CPacket& packet) override;
  void onGameStateChange(const GameStateChangeS2CPacket& packet) override;
  void onChunkStatusUpdate(const ChunkStatusUpdateS2CPacket& packet) override;
  void handleChunkData(const ChunkDataS2CPacket& packet) override;
  void onChunkDeltaUpdate(const ChunkDeltaUpdateS2CPacket& packet) override;
  void onBlockUpdate(const BlockUpdateS2CPacket& packet) override;
  void onEntityEquipmentUpdate(const EntityEquipmentUpdateS2CPacket& packet) override;
  void onEntityDestroy(const EntityDestroyS2CPacket& packet) override;
  void onEntityPosition(const EntityPositionS2CPacket& packet) override;
  void onEntityStatus(const EntityStatusS2CPacket& packet) override;
  void onEntityVehicleSet(const EntityVehicleSetS2CPacket& packet) override;
  void onEntityTrackerUpdate(const EntityTrackerUpdateS2CPacket& packet) override;
  void onEntityVelocityUpdate(const EntityVelocityUpdateS2CPacket& packet) override;
  void onItemEntitySpawn(const ItemEntitySpawnS2CPacket& packet) override;
  void onItemPickupAnimation(const ItemPickupAnimationS2CPacket& packet) override;
  void onLivingEntitySpawn(const LivingEntitySpawnS2CPacket& packet) override;
  void onPaintingEntitySpawn(const PaintingEntitySpawnS2CPacket& packet) override;
  void onPlayNoteSound(const PlayNoteSoundS2CPacket& packet) override;
  void onExplosion(const ExplosionS2CPacket& packet) override;
  void onLightningEntitySpawn(const GlobalEntitySpawnS2CPacket& packet) override;
  void onIncreaseStat(const IncreaseStatS2CPacket& packet) override;
  void onMapUpdate(const MapUpdateS2CPacket& packet) override;
  void onWorldEvent(const WorldEventS2CPacket& packet) override;
  void onOpenScreen(const OpenScreenS2CPacket& packet) override;
  void onCloseScreen(const CloseScreenS2CPacket& packet) override;
  void onScreenHandlerSlotUpdate(const ScreenHandlerSlotUpdateS2CPacket& packet) override;
  void onInventory(const InventoryS2CPacket& packet) override;
  void onScreenHandlerPropertyUpdate(const ScreenHandlerPropertyUpdateS2CPacket& packet) override;
  void onScreenHandlerAcknowledgement(const ScreenHandlerAcknowledgementPacket& packet) override;
  void onPlayerSleepUpdate(const PlayerSleepUpdateS2CPacket& packet) override;
  void onEntityAnimation(const EntityAnimationPacket& packet) override;
  void handleUpdateSign(const UpdateSignPacket& packet) override;
  void applyDeferredRespawn();
  std::string message;
  bool disconnected = false;
  bool started = false;
  client::Minecraft* minecraft = nullptr;
  World* world = nullptr;
  JavaRandom random;

private:
  [[nodiscard]] Entity* getEntity(int id);
  void processPendingJoinServer();
  bool pendingRespawn_ = false;
  int pendingRespawnDimension_ = 0;
  void setEntityPositionAndAnglesAvoidEntities(entity::Entity* entity, double x, double y, double z, float yaw,
                                               float pitch, int steps);
  // Move the live ClientWorld out of ownedWorld_ without freeing it yet. Packet
  // handlers run inside ClientWorld::tick() (via networkHandler_->tick()), so
  // destroying the world here would free the object whose tick() is on the stack.
  // Retired worlds are released at the top of tick(), once that stack has unwound.
  void retireOwnedWorld() {
    if(ownedWorld_ != nullptr) {
      retiredWorlds_.push_back(std::move(ownedWorld_));
    }
  }
  Connection* connection_ = nullptr;
  std::unique_ptr<ClientWorld> ownedWorld_;
  std::vector<std::unique_ptr<ClientWorld>> retiredWorlds_;
  std::unique_ptr<SimpleInventory> openScreenInventory_;
  std::unique_ptr<block::entity::FurnaceBlockEntity> openScreenFurnace_;
  std::unique_ptr<block::entity::DispenserBlockEntity> openScreenDispenser_;
  enum class JoinServerState {
    None,
    Pending,
    Succeeded,
    Failed,
  };
  JoinServerState joinServerState_ = JoinServerState::None;
  auth::JoinServerResult joinServerResult_{};
  std::mutex joinServerMutex_{};
  std::thread joinServerThread_{};
  std::atomic_bool joinServerCanceled_{false};
  enum class RemoteServerKind {
    JavaCompatible,
    NativeCppMods
  };
  void beginPendingLogin(const std::string& serverId);
  [[nodiscard]] std::vector<std::string> activeClientMods() const;
  RemoteServerKind remoteServerKind_ = RemoteServerKind::JavaCompatible;
  bool remoteLuaModsEnabled_ = false;
  bool waitingForModDownloadAcceptance_ = false;
  std::string pendingServerId_;
  std::vector<std::string> pendingMissingMods_;
  std::vector<std::string> pendingRequiredMods_;
  std::unordered_map<std::string, std::string> pendingDownloadUrls_;
};
} // namespace net::minecraft::client::multiplayer
