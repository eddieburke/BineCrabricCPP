#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class ArrowItem : public Item {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 6;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    ArrowItem();
};

} // namespace net::minecraft::item
