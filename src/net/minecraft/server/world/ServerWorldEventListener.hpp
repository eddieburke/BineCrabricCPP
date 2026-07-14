#pragma once
#include "net/minecraft/world/events/GameEventListener.hpp"
namespace net::minecraft {
class ServerWorld;
namespace entity {
class Entity;
class PlayerEntity;
} // namespace entity
namespace block::entity {
class BlockEntity;
}
namespace server {
class MinecraftServer;
namespace world {
class ServerWorldEventListener : public GameEventListener {
public:
  ServerWorldEventListener(MinecraftServer* server, ServerWorld* world);
  void notifyEntityAdded(Entity* entity) override;
  void notifyEntityRemoved(Entity* entity) override;
  void onEntityPickup(Entity* entity, PlayerEntity* collector) override;
  void blockUpdate(int x, int y, int z) override;
  void updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity) override;
  void worldEvent(PlayerEntity* player, int event, int x, int y, int z, int data) override;

private:
  MinecraftServer* server_ = nullptr;
  ServerWorld* world_ = nullptr;
};
} // namespace world
} // namespace server
} // namespace net::minecraft
