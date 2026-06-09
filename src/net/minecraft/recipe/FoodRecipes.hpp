#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>

namespace net::minecraft::recipe {

class FoodRecipes {
public:
    void add(CraftingRecipeManager& recipeManager)
    {
        recipeManager.addShapedRecipe(ItemStack(Item::MUSHROOM_STEW),
            {std::string("Y"), std::string("X"), std::string("#"), 'X', Block::BROWN_MUSHROOM, 'Y', Block::RED_MUSHROOM, '#', Item::BOWL});
        recipeManager.addShapedRecipe(ItemStack(Item::MUSHROOM_STEW),
            {std::string("Y"), std::string("X"), std::string("#"), 'X', Block::RED_MUSHROOM, 'Y', Block::BROWN_MUSHROOM, '#', Item::BOWL});
        recipeManager.addShapedRecipe(ItemStack(Item::COOKIE, 8),
            {std::string("#X#"), 'X', ItemStack(Item::DYE, 1, 3), '#', Item::WHEAT});
    }
};

} // namespace net::minecraft::recipe
