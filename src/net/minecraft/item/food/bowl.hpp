#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class BowlItem : public Item {
public:
    static constexpr int kRawId = 25;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    BowlItem();
};

} // namespace net::minecraft::item
