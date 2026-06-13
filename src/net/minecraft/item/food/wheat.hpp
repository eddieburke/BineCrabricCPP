#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class WheatItem : public Item {
public:
    static constexpr int kRawId = 40;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    WheatItem();
};

} // namespace net::minecraft::item
