#pragma once
#include "net/minecraft/block/TorchBlock.hpp"
namespace net::minecraft {
class BlockView;
class World;
} // namespace net::minecraft
namespace net::minecraft::recipe {
class CraftingRecipeManager;
}
namespace net::minecraft::block {
class RedstoneTorchBlock : public TorchBlock {
 public:
 static constexpr int kBlockId = 76;
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

 public:
 static void registerClass();
 bool lit = false;
 RedstoneTorchBlock(int id, int textureId, bool litIn);
 [[nodiscard]] int getTickRate() const override {
  return 2;
 }
 [[nodiscard]] int getTexture(int side, int meta) const override;
 [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override;
 [[nodiscard]] bool canEmitRedstonePower() const override {
  return true;
 }
 void randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) override;
 void onPlaced(World* world, int x, int y, int z) override;
 void onBreak(World* world, int x, int y, int z) override;
 void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
 void neighborUpdate(World* world, int x, int y, int z, int id) override;
 [[nodiscard]] bool canTransferPowerInDirection(World* world, int x, int y, int z, int direction) const override;
 [[nodiscard]] bool isEmittingRedstonePowerInDirection(
     const BlockView* blockView, int x, int y, int z, int direction) const override;

 private:
 [[nodiscard]] bool shouldUnpower(World* world, int x, int y, int z) const;
 [[nodiscard]] bool isBurnedOut(World* world, int x, int y, int z, bool addNew);
};
} // namespace net::minecraft::block
