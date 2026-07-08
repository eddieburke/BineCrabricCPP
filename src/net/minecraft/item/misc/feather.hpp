#pragma once
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class FeatherItem : public Item {
   public:
    static constexpr int kRawId = 32;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    FeatherItem();
};
}  // namespace net::minecraft::item
