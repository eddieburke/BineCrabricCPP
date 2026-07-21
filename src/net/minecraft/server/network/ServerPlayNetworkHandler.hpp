#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/server/command/CommandOutput.hpp"
namespace net::minecraft {
class BlockUpdateS2CPacket;
class ChatMessagePacket;
class ClickSlotC2SPacket;
class ClientCommandC2SPacket;
class CloseScreenS2CPacket;
class DisconnectPacket;
class EntityAnimationPacket;
class PlayerActionC2SPacket;
class PlayerInputC2SPacket;
class PlayerInteractBlockC2SPacket;
class PlayerInteractEntityC2SPacket;
class PlayerMovePacket;
class PlayerRespawnPacket;
class ScreenHandlerAcknowledgementPacket;
class ScreenHandlerSlotUpdateS2CPacket;
class UpdateSelectedSlotC2SPacket;
class UpdateSignPacket;
namespace entity::player {
class ServerPlayerEntity;
}
namespace server {
class MinecraftServer;
namespace network {
class ServerPlayNetworkHandler : public NetworkHandler, public command::CommandOutput {
 public:
 ServerPlayNetworkHandler(MinecraftServer* server,
                          Connection* connection,
                          ::net::minecraft::entity::player::ServerPlayerEntity* player);
 void tick();
 void disconnect(const std::string& reason);
 void setPlayer(::net::minecraft::entity::player::ServerPlayerEntity* player);
 template <typename PacketT>
 void sendPacket(const PacketT& packet) {
  if(connection_ != nullptr) {
   connection_->sendPacket(std::make_unique<PacketT>(packet));
  }
 }
 void sendPacket(std::unique_ptr<Packet> packet) {
  if(connection_ != nullptr && packet) {
   connection_->sendPacket(std::move(packet));
  }
 }
 void teleport(double x, double y, double z, float yaw, float pitch);
 [[nodiscard]] std::size_t getBlockDataSendQueueSize() const;
 [[nodiscard]] bool isServerSide() const override {
  return true;
 }
 void sendMessage(const std::string& message) override;
 [[nodiscard]] std::string getName() override;
 void onPlayerInput(const PlayerInputC2SPacket& packet) override;
 void onPlayerMove(const PlayerMovePacket& packet) override;
 void handleClientCommand(const ClientCommandC2SPacket& packet) override;
 void handlePlayerAction(const PlayerActionC2SPacket& packet) override;
 void onPlayerInteractBlock(const PlayerInteractBlockC2SPacket& packet) override;
 void onDisconnected(const std::string& reason, const std::vector<std::string>& args) override;
 void onKeepAlive(const KeepAlivePacket& packet) override;
 void handle(const Packet& packet) override;
 void onUpdateSelectedSlot(const UpdateSelectedSlotC2SPacket& packet) override;
 void onChatMessage(const ChatMessagePacket& packet) override;
 void onEntityAnimation(const EntityAnimationPacket& packet) override;
 void onDisconnect(const DisconnectPacket& packet) override;
 void handleInteractEntity(const PlayerInteractEntityC2SPacket& packet) override;
 void onPlayerRespawn(const PlayerRespawnPacket& packet) override;
 void onCloseScreen(const CloseScreenS2CPacket& packet) override;
 void onClickSlot(const ClickSlotC2SPacket& packet) override;
 void onScreenHandlerAcknowledgement(const ScreenHandlerAcknowledgementPacket& packet) override;
 void handleUpdateSign(const UpdateSignPacket& packet) override;
 bool disconnected = false;

 private:
 void handleCommand(const std::string& message);
 MinecraftServer* server_ = nullptr;
 Connection* connection_ = nullptr;
 ::net::minecraft::entity::player::ServerPlayerEntity* player_ = nullptr;
 int ticks_ = 0;
 int lastKeepAliveTime_ = 0;
 int floatingTime_ = 0;
 bool moved_ = false;
 double teleportTargetX_ = 0.0;
 double teleportTargetY_ = 0.0;
 double teleportTargetZ_ = 0.0;
 bool teleported_ = true;
 std::unordered_map<int, std::int16_t> transactions_;
};
} // namespace network
} // namespace server
} // namespace net::minecraft
