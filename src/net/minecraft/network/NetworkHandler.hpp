#pragma once
#include <string>
#include <vector>
#include "net/minecraft/network/Packet.hpp"
namespace net::minecraft {
class BlockUpdateS2CPacket;
class ChatMessagePacket;
class ChunkDataS2CPacket;
class ChunkDeltaUpdateS2CPacket;
class ChunkStatusUpdateS2CPacket;
class ClickSlotC2SPacket;
class ClientCommandC2SPacket;
class CloseScreenS2CPacket;
class DisconnectPacket;
class EntityDestroyS2CPacket;
class EntityEquipmentUpdateS2CPacket;
class EntityAnimationPacket;
class EntityMoveRelativeS2CPacket;
class EntityPositionS2CPacket;
class EntityRotateAndMoveRelativeS2CPacket;
class EntityRotateS2CPacket;
class EntityS2CPacket;
class EntitySpawnS2CPacket;
class EntityStatusS2CPacket;
class EntityTrackerUpdateS2CPacket;
class EntityVehicleSetS2CPacket;
class EntityVelocityUpdateS2CPacket;
class ExplosionS2CPacket;
class GameStateChangeS2CPacket;
class HandshakePacket;
class HealthUpdateS2CPacket;
class InventoryS2CPacket;
class IncreaseStatS2CPacket;
class ItemEntitySpawnS2CPacket;
class ItemPickupAnimationS2CPacket;
class LivingEntitySpawnS2CPacket;
class GlobalEntitySpawnS2CPacket;
class MapUpdateS2CPacket;
class KeepAlivePacket;
class LoginHelloPacket;
class LuaModSyncPacket;
class OpenScreenS2CPacket;
class PaintingEntitySpawnS2CPacket;
class PlayNoteSoundS2CPacket;
class PlayerActionC2SPacket;
class PlayerInputC2SPacket;
class PlayerInteractBlockC2SPacket;
class PlayerInteractEntityC2SPacket;
class PlayerMovePacket;
class PlayerRespawnPacket;
class PlayerSleepUpdateS2CPacket;
class PlayerSpawnPositionS2CPacket;
class PlayerSpawnS2CPacket;
class ScreenHandlerAcknowledgementPacket;
class ScreenHandlerPropertyUpdateS2CPacket;
class ScreenHandlerSlotUpdateS2CPacket;
class UpdateSelectedSlotC2SPacket;
class UpdateSignPacket;
class WorldEventS2CPacket;
class WorldTimeUpdateS2CPacket;
class NetworkHandler {
public:
  virtual ~NetworkHandler() = default;
  [[nodiscard]] virtual bool isServerSide() const = 0;
  // Set true once the connection has negotiated the lua-mod protocol (via the
  // login handshake). Mod-only packets (custom tile-entity / entity sync) are
  // sent and parsed only when this is true, so they never reach a vanilla
  // server or client that does not speak the mod protocol.
  [[nodiscard]] bool modProtocolEnabled() const noexcept {
    return modProtocolEnabled_;
  }
  void setModProtocolEnabled(bool value) noexcept {
    modProtocolEnabled_ = value;
  }
  virtual void handle(const Packet&) {
  }
  virtual void onDisconnected(const std::string&, const std::vector<std::string>&) {
  }
  virtual void onHandshake(const HandshakePacket&) {
  }
  virtual void onHello(const LoginHelloPacket&) {
  }
  virtual void onLuaModSync(const LuaModSyncPacket&) {
  }
  virtual void onChatMessage(const ChatMessagePacket&) {
  }
  virtual void onDisconnect(const DisconnectPacket&) {
  }
  virtual void onKeepAlive(const KeepAlivePacket&) {
  }
  virtual void onEntity(const EntityS2CPacket&) {
  }
  virtual void onEntitySpawn(const EntitySpawnS2CPacket&) {
  }
  virtual void onPlayerMove(const PlayerMovePacket&) {
  }
  virtual void onPlayerRespawn(const PlayerRespawnPacket&) {
  }
  virtual void onPlayerSpawn(const PlayerSpawnS2CPacket&) {
  }
  virtual void onPlayerSpawnPosition(const PlayerSpawnPositionS2CPacket&) {
  }
  virtual void onWorldTimeUpdate(const WorldTimeUpdateS2CPacket&) {
  }
  virtual void onHealthUpdate(const HealthUpdateS2CPacket&) {
  }
  virtual void onGameStateChange(const GameStateChangeS2CPacket&) {
  }
  virtual void onChunkStatusUpdate(const ChunkStatusUpdateS2CPacket&) {
  }
  virtual void handleChunkData(const ChunkDataS2CPacket&) {
  }
  virtual void onChunkDeltaUpdate(const ChunkDeltaUpdateS2CPacket&) {
  }
  virtual void onBlockUpdate(const BlockUpdateS2CPacket&) {
  }
  virtual void onEntityEquipmentUpdate(const EntityEquipmentUpdateS2CPacket&) {
  }
  virtual void onEntityDestroy(const EntityDestroyS2CPacket&) {
  }
  virtual void onEntityMoveRelative(const EntityMoveRelativeS2CPacket&) {
  }
  virtual void onEntityRotate(const EntityRotateS2CPacket&) {
  }
  virtual void onEntityRotateAndMoveRelative(const EntityRotateAndMoveRelativeS2CPacket&) {
  }
  virtual void onEntityPosition(const EntityPositionS2CPacket&) {
  }
  virtual void onEntityStatus(const EntityStatusS2CPacket&) {
  }
  virtual void onEntityVehicleSet(const EntityVehicleSetS2CPacket&) {
  }
  virtual void onEntityTrackerUpdate(const EntityTrackerUpdateS2CPacket&) {
  }
  virtual void onEntityVelocityUpdate(const EntityVelocityUpdateS2CPacket&) {
  }
  virtual void onItemEntitySpawn(const ItemEntitySpawnS2CPacket&) {
  }
  virtual void onItemPickupAnimation(const ItemPickupAnimationS2CPacket&) {
  }
  virtual void onLivingEntitySpawn(const LivingEntitySpawnS2CPacket&) {
  }
  virtual void onPaintingEntitySpawn(const PaintingEntitySpawnS2CPacket&) {
  }
  virtual void onPlayNoteSound(const PlayNoteSoundS2CPacket&) {
  }
  virtual void onExplosion(const ExplosionS2CPacket&) {
  }
  virtual void onLightningEntitySpawn(const GlobalEntitySpawnS2CPacket&) {
  }
  virtual void onIncreaseStat(const IncreaseStatS2CPacket&) {
  }
  virtual void onMapUpdate(const MapUpdateS2CPacket&) {
  }
  virtual void onWorldEvent(const WorldEventS2CPacket&) {
  }
  virtual void onOpenScreen(const OpenScreenS2CPacket&) {
  }
  virtual void onCloseScreen(const CloseScreenS2CPacket&) {
  }
  virtual void onScreenHandlerSlotUpdate(const ScreenHandlerSlotUpdateS2CPacket&) {
  }
  virtual void onInventory(const InventoryS2CPacket&) {
  }
  virtual void onScreenHandlerPropertyUpdate(const ScreenHandlerPropertyUpdateS2CPacket&) {
  }
  virtual void onScreenHandlerAcknowledgement(const ScreenHandlerAcknowledgementPacket&) {
  }
  virtual void onPlayerSleepUpdate(const PlayerSleepUpdateS2CPacket&) {
  }
  virtual void onEntityAnimation(const EntityAnimationPacket&) {
  }
  virtual void handleClientCommand(const ClientCommandC2SPacket&) {
  }
  virtual void handlePlayerAction(const PlayerActionC2SPacket&) {
  }
  virtual void onPlayerInteractBlock(const PlayerInteractBlockC2SPacket&) {
  }
  virtual void handleInteractEntity(const PlayerInteractEntityC2SPacket&) {
  }
  virtual void onUpdateSelectedSlot(const UpdateSelectedSlotC2SPacket&) {
  }
  virtual void onPlayerInput(const PlayerInputC2SPacket&) {
  }
  virtual void onClickSlot(const ClickSlotC2SPacket&) {
  }
  virtual void handleUpdateSign(const UpdateSignPacket&) {
  }

private:
  bool modProtocolEnabled_ = false;
};
} // namespace net::minecraft
