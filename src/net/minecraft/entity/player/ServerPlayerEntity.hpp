#pragma once
#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_set>
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
#include "net/minecraft/server/network/ServerPlayerInteractionManager.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::server {
class MinecraftServer;
}
namespace net::minecraft::server::network {
class ServerPlayNetworkHandler;
class ServerPlayerInteractionManager;
} // namespace net::minecraft::server::network
namespace net::minecraft::entity::player {
class ServerPlayerEntity : public PlayerEntity, public screen::ScreenHandlerListener {
public:
  using LivingEntity::getEquipment;
  ServerPlayerEntity(server::MinecraftServer* server,
                     World* world,
                     const std::string& name,
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
  void resetEyeHeight() override;
  void setWorld(World* world);
  void initScreenHandler();
  void openChestScreen(Inventory* inventoryIn) override;
  void openChestScreen(int xIn, int yIn, int zIn) override;
  void openFurnaceScreen(::net::minecraft::block::entity::FurnaceBlockEntity* furnaceIn) override;
  void openDispenserScreen(::net::minecraft::block::entity::DispenserBlockEntity* dispenserIn) override;
  void openCraftingScreen(int xIn, int yIn, int zIn) override;
  void onSlotUpdate(screen::ScreenHandler& handler, int slot, const ItemStack& stack) override;
  void onContentsUpdate(screen::ScreenHandler& handler, const std::vector<ItemStack>& stacks) override;
  void onPropertyUpdate(screen::ScreenHandler& handler, int property, int value) override;
  void swingHand() override;
  void clearWakePosition();
  void markHealthDirty();
  void onContentsUpdate(screen::ScreenHandler& screenHandler);
  void onDisconnect();
  void setVehicle(Entity* entity) override;
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
  void incrementScreenHandlerSyncId();
  std::array<ItemStack, 5> equipment_{};
  std::unique_ptr<screen::ScreenHandler> ownedScreenHandler_;
  std::unique_ptr<Inventory> ownedScreenInventory_;
  int screenHandlerSyncId_ = 0;
};
} // namespace net::minecraft::entity::player
