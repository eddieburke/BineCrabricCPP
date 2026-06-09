#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>

namespace net::minecraft::recipe {

class BasicBlockRecipes {
public:
    void add(CraftingRecipeManager& recipeManager)
    {
        recipeManager.addShapedRecipe(ItemStack(Block::CHEST),
            {std::string("###"), std::string("# #"), std::string("###"), '#', Block::PLANKS});
        recipeManager.addShapedRecipe(ItemStack(Block::FURNACE),
            {std::string("###"), std::string("# #"), std::string("###"), '#', Block::COBBLESTONE});
        recipeManager.addShapedRecipe(ItemStack(Block::CRAFTING_TABLE),
            {std::string("##"), std::string("##"), '#', Block::PLANKS});
        recipeManager.addShapedRecipe(ItemStack(Block::SANDSTONE),
            {std::string("##"), std::string("##"), '#', Block::SAND});
    }
};

} // namespace net::minecraft::recipe
