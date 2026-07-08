#pragma once
#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft {
class World;
class ItemStack;
}  // namespace net::minecraft

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class MushroomStewItem : public FoodItem {
   public:
    static constexpr int kRawId = 26;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    MushroomStewItem(int rawId, int healthRestored);
    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;
};
}  // namespace net::minecraft::item
