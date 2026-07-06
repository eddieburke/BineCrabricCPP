#pragma once
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::server::network {
class ServerPlayerInteractionManager {
public:
  explicit ServerPlayerInteractionManager(World* world);
  void update();
  void onBlockBreakingAction(int x, int y, int z, int direction);
  void continueMining(int x, int y, int z);
  bool finishMining(int x, int y, int z);
  bool tryBreakBlock(int x, int y, int z);
  bool interactItem(::net::minecraft::entity::player::PlayerEntity* player, World* world, ItemStack* stack);
  bool interactBlock(::net::minecraft::entity::player::PlayerEntity* player, World* world, ItemStack* stack, int x, int y, int z,
                     int side);
  World* world = nullptr;
  ::net::minecraft::entity::player::PlayerEntity* player = nullptr;

private:
  float blockBreakProgress_ = 0.0f;
  int failedMiningStartTime_ = 0;
  int failedMiningX_ = 0;
  int failedMiningY_ = 0;
  int failedMiningZ_ = 0;
  int tickCounter_ = 0;
  bool mining_ = false;
  int miningX_ = 0;
  int miningY_ = 0;
  int miningZ_ = 0;
  int startMiningTime_ = 0;
};
} // namespace net::minecraft::server::network
