#pragma once
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/server/network/ServerPlayerInteractionManager.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include <array>
#include <deque>
#include <functional>
#include <unordered_set>
namespace net::minecraft::server {
class MinecraftServer;
}
namespace net::minecraft::server::network {
class ServerPlayNetworkHandler;
class ServerPlayerInteractionManager;
} // namespace net::minecraft::server::network
namespace net::minecraft::entity::player {
class ServerPlayerEntity : public PlayerEntity {
public:
  using LivingEntity::getEquipment;
  ServerPlayerEntity(server::MinecraftServer* server, World* world, const std::string& name,
                     server::network::ServerPlayerInteractionManager* interactionManager);
  void tick() override;
  void playerTick(bool shouldSendChunkUpdates);
  void onKilledBy(Entity* adversary) override;
  bool damage(Entity* damageSource, int amount) override;
  bool isPvpEnabled() const override;
  void respawn() override;
  SleepAttemptResult trySleep(int xIn, int yIn, int zIn);
  void wakeUp(bool resetSleepTimer, bool updateSleepingPlayers, bool setSpawnPosFlag);
  void increaseStat(int stat, int amount) override;
  void sendMessage(const std::string& message) override;
  void updateInput(float sidewaysSpeed, float forwardSpeed, bool jumping, bool sneaking, float pitchIn, float yawIn);
  void handleFall(double heightDifference, bool onGroundIn);
  [[nodiscard]] float getEyeHeight() const override;
  void setWorld(World* world);
  void initScreenHandler();
  void clearWakePosition();
  void markHealthDirty();
  void onContentsUpdate(screen::ScreenHandler& screenHandler);
  void onContentsUpdate(screen::ScreenHandler& screenHandler, const std::vector<ItemStack>& stacks);
  void onDisconnect();
  void onHandledScreenClosed();
  void updateCursorStack();
  void closeHandledScreen() override;
  using StatPacketSender = std::function<void(int statId, int amount)>;
  StatPacketSender statPacketSender;
  [[nodiscard]] ItemStack getEquipment(int slot) const;
  server::MinecraftServer* server = nullptr;
  server::network::ServerPlayNetworkHandler* networkHandler = nullptr;
  server::network::ServerPlayerInteractionManager interactionManager;
  double lastX = 0.0;
  double lastZ = 0.0;
  std::deque<ChunkPos> pendingChunkUpdates;
  std::unordered_set<ChunkPos, ChunkPosHash> activeChunks;
  int joinInvulnerabilityTicks = 60;
  int lastHealthScore = -99999999;
  bool skipPacketSlotUpdates = false;

private:
  std::array<ItemStack, 5> equipment_{};
};
} // namespace net::minecraft::entity::player
