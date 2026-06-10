#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"

namespace net::minecraft::recipe {

SmeltingRecipeManager& SmeltingRecipeManager::instance()
{
    static SmeltingRecipeManager manager;
    return manager;
}

SmeltingRecipeManager::SmeltingRecipeManager() = default;

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
