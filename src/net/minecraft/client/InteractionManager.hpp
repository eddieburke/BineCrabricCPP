#pragma once
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::entity::player {
class ClientPlayerEntity;
}
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client {
class InteractionManager {
public:
  explicit InteractionManager(Minecraft* minecraft);
  virtual ~InteractionManager() = default;
  virtual void setWorld(World* world);
  virtual void attackBlock(int x, int y, int z, int direction);
  virtual bool breakBlock(int x, int y, int z, int direction);
  virtual void processBlockBreakingAction(int x, int y, int z, int side);
  virtual void cancelBlockBreaking();
  virtual void update(float partialTick);
  virtual float getReachDistance();
  virtual bool interactItem(PlayerEntity* player, World* world, ItemStack* item);
  virtual void preparePlayer(PlayerEntity* player);
  virtual void tick();
  [[nodiscard]] virtual float getBlockBreakingProgress(float partialTick) const;
  [[nodiscard]] virtual float getLastBlockBreakingProgress() const;
  virtual bool canBeRendered();
  virtual void preparePlayerRespawn(PlayerEntity* player);
  virtual bool interactBlock(PlayerEntity* player, World* world, ItemStack* item, int x, int y, int z, int side);
  virtual PlayerEntity* createPlayer(World* world);
  virtual void interactEntity(PlayerEntity* player, Entity* entity);
  virtual void attackEntity(PlayerEntity* player, Entity* target);
  virtual ItemStack* clickSlot(int syncId, int slotId, int button, bool shift, PlayerEntity* player);
  virtual void onScreenRemoved(int syncId, PlayerEntity* player);
  Minecraft* minecraft = nullptr;
  bool noTick = false;

protected:
  ItemStack lastClickedStack_{};
};
} // namespace net::minecraft::client
