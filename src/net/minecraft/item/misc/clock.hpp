#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class ClockItem : public Item {
public:
    static constexpr int kRawId = 91;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    ClockItem();
};

} // namespace net::minecraft::item
