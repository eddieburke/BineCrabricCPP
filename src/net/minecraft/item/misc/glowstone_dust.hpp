#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class GlowstoneDustItem : public Item {
public:
    static constexpr int kRawId = 92;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    GlowstoneDustItem();
};

} // namespace net::minecraft::item
