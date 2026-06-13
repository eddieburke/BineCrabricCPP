#pragma once

#include "net/minecraft/item/HoeItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class WoodenHoeItem : public HoeItem {
public:
    static constexpr int kRawId = 34;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    WoodenHoeItem();
};

} // namespace net::minecraft::item
