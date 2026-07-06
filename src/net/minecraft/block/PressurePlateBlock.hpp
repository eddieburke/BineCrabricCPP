#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PressurePlateActivationRule.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
}
namespace net::minecraft::block {
class PressurePlateBlock : public Block {
public:
  static constexpr int kBlockId = 72;
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

public:
  static void registerClass();
  PressurePlateActivationRule activationRule = PressurePlateActivationRule::EVERYTHING;
  PressurePlateBlock(int id, int textureId, PressurePlateActivationRule activationRule, Material& material);
  [[nodiscard]] bool isOpaque() const override {
    return false;
  }
  [[nodiscard]] bool isFullCube() const override {
    return false;
  }
  [[nodiscard]] int getTickRate() const override {
    return 20;
  }
  [[nodiscard]] int getPistonBehavior() const {
    return 1;
  }
  [[nodiscard]] bool canEmitRedstonePower() const override {
    return true;
  }
  [[nodiscard]] bool isEmittingRedstonePowerInDirection(const BlockView* blockView, int x, int y, int z,
                                                        int /*direction*/) const override;
  [[nodiscard]] bool canTransferPowerInDirection(World* world, int x, int y, int z, int direction) const override;
  [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/, int /*x*/, int /*y*/,
                                                                     int /*z*/) const override {
    return std::nullopt;
  }
  [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const override;
  void neighborUpdate(World* world, int x, int y, int z, int id) override;
  void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
  void onEntityCollision(World* world, int x, int y, int z, net::minecraft::Entity* entity) override;
  void onBreak(World* world, int x, int y, int z) override;
  void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
  [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;

private:
  void updatePlateState(World* world, int x, int y, int z);
};
} // namespace net::minecraft::block
