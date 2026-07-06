#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class BedItem : public Item {
public:
  static constexpr int kRawId = 99;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  BedItem();
  bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;
};
} // namespace net::minecraft::item
