#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {

SmeltingRecipeManager& SmeltingRecipeManager::instance()
{
    static SmeltingRecipeManager manager;
    return manager;
}

SmeltingRecipeManager::SmeltingRecipeManager()
{
    addRecipe(Block::IRON_ORE->id, ItemStack(Item::IRON_INGOT));
    addRecipe(Block::GOLD_ORE->id, ItemStack(Item::GOLD_INGOT));
    addRecipe(Block::DIAMOND_ORE->id, ItemStack(Item::DIAMOND));
    addRecipe(Block::SAND->id, ItemStack(Block::GLASS));
    addRecipe(Item::RAW_PORKCHOP->id, ItemStack(Item::COOKED_PORKCHOP));
    addRecipe(Item::RAW_FISH->id, ItemStack(Item::COOKED_FISH));
    addRecipe(Block::COBBLESTONE->id, ItemStack(Block::STONE));
    addRecipe(Item::CLAY->id, ItemStack(Item::BRICK));
    addRecipe(Block::CACTUS->id, ItemStack(Item::DYE, 1, 2));
    addRecipe(Block::LOG->id, ItemStack(Item::COAL, 1, 1));
}

void SmeltingRecipeManager::addRecipe(int inputId, ItemStack output)
{
    recipes_[inputId] = std::move(output);
}

std::optional<ItemStack> SmeltingRecipeManager::craft(int inputId) const
{
    const auto it = recipes_.find(inputId);
    if (it == recipes_.end()) {
        return std::nullopt;
    }
    return it->second.copy();
}

} // namespace net::minecraft::recipe
