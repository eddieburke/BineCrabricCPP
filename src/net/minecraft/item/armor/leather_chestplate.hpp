#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class LeatherChestplateItem : public ArmorItem {
public:
    static constexpr int kRawId = 43;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    LeatherChestplateItem();
};

} // namespace net::minecraft::item
