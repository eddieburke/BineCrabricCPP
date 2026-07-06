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
class FlintAndSteel : public Item {
public:
  static constexpr int kRawId = 3;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  explicit FlintAndSteel(int rawId);
  bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;
};
} // namespace net::minecraft::item
