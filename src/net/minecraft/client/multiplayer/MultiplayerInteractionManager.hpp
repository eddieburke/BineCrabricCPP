#pragma once
#include "net/minecraft/client/InteractionManager.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
namespace net::minecraft::client {
class MultiplayerInteractionManager : public InteractionManager {
public:
  MultiplayerInteractionManager(Minecraft* minecraft, multiplayer::ClientNetworkHandler* networkHandler);
  void preparePlayer(PlayerEntity* player);
  bool breakBlock(int x, int y, int z, int direction);
  void attackBlock(int x, int y, int z, int direction);
  void cancelBlockBreaking();
  void processBlockBreakingAction(int x, int y, int z, int side);
  void update(float partialTick);
  float getReachDistance();
  void setWorld(World* world);
  void tick();
  bool interactBlock(PlayerEntity* player, World* world, ItemStack* item, int x, int y, int z, int side);
  bool interactItem(PlayerEntity* player, World* world, ItemStack* item);
  PlayerEntity* createPlayer(World* world);
  void attackEntity(PlayerEntity* player, Entity* target);
  void interactEntity(PlayerEntity* player, Entity* entity);
  ItemStack* clickSlot(int syncId, int slotId, int button, bool shift, PlayerEntity* player);
  void onScreenRemoved(int syncId, PlayerEntity* player);

private:
  void updateSelectedSlot();
  int breakingPosX = -1;
  int breakingPosY = -1;
  int breakingPosZ = -1;
  float blockBreakingProgress = 0.0f;
  float lastBlockBreakingProgress = 0.0f;
  float breakingSoundDelayTicks = 0.0f;
  int breakingDelayTicks = 0;
  bool breakingBlock = false;
  multiplayer::ClientNetworkHandler* networkHandler = nullptr;
  int selectedSlot = 0;
};
} // namespace net::minecraft::client
