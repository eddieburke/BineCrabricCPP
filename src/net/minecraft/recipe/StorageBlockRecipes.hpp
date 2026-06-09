#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>
#include <vector>

namespace net::minecraft::recipe {

class StorageBlockRecipes {
public:
    void add(CraftingRecipeManager& recipeManager)
    {
        for (const auto& entry : entries) {
            Block* block = entry.block;
            ItemStack stack = entry.stack;
            recipeManager.addShapedRecipe(ItemStack(block),
                {std::string("###"), std::string("###"), std::string("###"), '#', stack});
            recipeManager.addShapedRecipe(stack, {std::string("#"), '#', block});
        }
    }

private:
    struct Entry {
        Block* block;
        ItemStack stack;
    };

    std::vector<Entry> entries{
        {Block::GOLD_BLOCK, ItemStack(Item::GOLD_INGOT, 9)},
        {Block::IRON_BLOCK, ItemStack(Item::IRON_INGOT, 9)},
        {Block::DIAMOND_BLOCK, ItemStack(Item::DIAMOND, 9)},
        {Block::LAPIS_BLOCK, ItemStack(Item::DYE, 9, 4)}};
};

} // namespace net::minecraft::recipe
