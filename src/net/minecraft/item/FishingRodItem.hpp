#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class FishingRodItem : public Item {
public:
  static constexpr int kRawId = 90;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  explicit FishingRodItem(int rawId);
  [[nodiscard]] bool isHandheldRod() const override;
  ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;
};
} // namespace net::minecraft::item
