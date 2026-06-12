#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class GoldenChestplateItem : public ArmorItem {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 59;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    GoldenChestplateItem();
};

} // namespace net::minecraft::item
