#pragma once
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class CompassItem : public Item {
   public:
    static constexpr int kRawId = 89;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    CompassItem();
};
}  // namespace net::minecraft::item
