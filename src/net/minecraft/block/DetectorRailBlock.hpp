#pragma once
#include "net/minecraft/block/RailBlock.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
}
namespace net::minecraft::block {
class DetectorRailBlock : public RailBlock {
 public:
 static constexpr int kBlockId = 28;
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

 public:
 static void registerClass();
 DetectorRailBlock(int id, int textureId);
 [[nodiscard]] int getTickRate() const override {
  return 20;
 }
 [[nodiscard]] bool canEmitRedstonePower() const override {
  return true;
 }
 [[nodiscard]] bool isEmittingRedstonePowerInDirection(
     const BlockView* blockView, int x, int y, int z, int /*direction*/) const override {
  return blockView != nullptr && (blockView->getBlockMeta(x, y, z) & 8) != 0;
 }
 [[nodiscard]] bool canTransferPowerInDirection(World* world, int x, int y, int z, int direction) const override;
 void onEntityCollision(World* world, int x, int y, int z, net::minecraft::Entity* entity) override;
 void onTick(World* world, int x, int y, int z, JavaRandom& random) override;

 private:
 void updatePoweredStatus(World* world, int x, int y, int z, int meta);
};
} // namespace net::minecraft::block