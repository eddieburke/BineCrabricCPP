#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/WoolBlock.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

namespace net::minecraft::recipe {

class DyeRecipes {
public:
    void add(CraftingRecipeManager& recipeManager)
    {
        for (int i = 0; i < 16; ++i) {
            recipeManager.addShapelessRecipe(ItemStack(Block::WOOL, 1, block::WoolBlock::getItemMeta(i)),
                {ItemStack(Item::DYE, 1, i), ItemStack(Block::WOOL, 1, 0)});
        }
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 11), {Block::DANDELION});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 1), {Block::ROSE});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 3, 15), {Item::BONE});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 9),
            {ItemStack(Item::DYE, 1, 1), ItemStack(Item::DYE, 1, 15)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 14),
            {ItemStack(Item::DYE, 1, 1), ItemStack(Item::DYE, 1, 11)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 10),
            {ItemStack(Item::DYE, 1, 2), ItemStack(Item::DYE, 1, 15)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 8),
            {ItemStack(Item::DYE, 1, 0), ItemStack(Item::DYE, 1, 15)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 7),
            {ItemStack(Item::DYE, 1, 8), ItemStack(Item::DYE, 1, 15)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 3, 7),
            {ItemStack(Item::DYE, 1, 0), ItemStack(Item::DYE, 1, 15), ItemStack(Item::DYE, 1, 15)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 12),
            {ItemStack(Item::DYE, 1, 4), ItemStack(Item::DYE, 1, 15)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 6),
            {ItemStack(Item::DYE, 1, 4), ItemStack(Item::DYE, 1, 2)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 5),
            {ItemStack(Item::DYE, 1, 4), ItemStack(Item::DYE, 1, 1)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 2, 13),
            {ItemStack(Item::DYE, 1, 5), ItemStack(Item::DYE, 1, 9)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 3, 13),
            {ItemStack(Item::DYE, 1, 4), ItemStack(Item::DYE, 1, 1), ItemStack(Item::DYE, 1, 9)});
        recipeManager.addShapelessRecipe(ItemStack(Item::DYE, 4, 13),
            {ItemStack(Item::DYE, 1, 4), ItemStack(Item::DYE, 1, 1), ItemStack(Item::DYE, 1, 1), ItemStack(Item::DYE, 1, 15)});
    }
};

} // namespace net::minecraft::recipe
