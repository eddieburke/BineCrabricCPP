#pragma once
#include "net/minecraft/client/InteractionManager.hpp"
namespace net::minecraft::client {
class SingleplayerInteractionManager : public InteractionManager {
public:
  explicit SingleplayerInteractionManager(Minecraft* minecraft);
  void preparePlayer(PlayerEntity* player);
  bool breakBlock(int x, int y, int z, int direction);
  void attackBlock(int x, int y, int z, int direction);
  void cancelBlockBreaking();
  void processBlockBreakingAction(int x, int y, int z, int side);
  void update(float partialTick);
  float getReachDistance();
  void setWorld(World* world);
  void tick();
  [[nodiscard]] float getBlockBreakingProgress(float partialTick) const override;
  [[nodiscard]] float getLastBlockBreakingProgress() const override;

private:
  int breakingPosX = -1;
  int breakingPosY = -1;
  int breakingPosZ = -1;
  float blockBreakingProgress = 0.0f;
  float lastBlockBreakingProgress = 0.0f;
  float breakingSoundDelayTicks = 0.0f;
  int breakingDelayTicks = 0;
};
} // namespace net::minecraft::client
