#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class PaperItem : public Item {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 83;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    PaperItem();
};

} // namespace net::minecraft::item
