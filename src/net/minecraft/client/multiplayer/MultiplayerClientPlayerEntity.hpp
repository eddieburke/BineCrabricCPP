#pragma once
#include <cstdint>
#include <string>
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::util {
class Session;
}
namespace net::minecraft::client::multiplayer {
class MultiplayerClientPlayerEntity : public entity::player::ClientPlayerEntity {
public:
  MultiplayerClientPlayerEntity(client::Minecraft* minecraft,
                                World* world,
                                const client::util::Session& session,
                                ClientNetworkHandler* networkHandler);
  bool damage(Entity* damageSource, int amount);
  void heal(int amount);
  void tick();
  void sendMovementPackets();
  void dropSelectedItem();
  void sendChatMessage(const std::string& message) override;
  void swingHand();
  void respawn();
  void closeHandledScreen();
  void damageTo(int health);
  void increaseStat(int stat, int amount);
  void handleIncreaseStat(int stat, int amount);
  ClientNetworkHandler* networkHandler = nullptr;

protected:
  void spawnItem(ItemEntity* itemEntity);
  void applyDamage(int amount);

private:
  void syncStateBeforeRespawn();
  bool waitingForTerrainSupport_ = true;
  int packetSendCounter = 0;
  bool prevJumping = false;
  double lastSentX = 0.0;
  double lastSentBbMinY = 0.0;
  double lastSentY = 0.0;
  double lastSentZ = 0.0;
  float lastSentYaw = 0.0f;
  float lastSentPitch = 0.0f;
  bool lastOnGround = false;
  bool lastSneaking = false;
  int onGroundTicks = 0;
};
} // namespace net::minecraft::client::multiplayer
