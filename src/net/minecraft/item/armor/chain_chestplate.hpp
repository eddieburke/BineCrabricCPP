#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class ChainChestplateItem : public ArmorItem {
public:    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    ChainChestplateItem();
};

} // namespace net::minecraft::item
