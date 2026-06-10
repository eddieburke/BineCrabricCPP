#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::recipe {

struct CraftingRecipeBootstrap {
    static void registerClass()
    {
        CraftingRecipeManager::registerVanillaRecipes();
    }
};

static registry::RegisterCustom<CraftingRecipeBootstrap> s_reg(registry::kCraftingRecipeRegistrarBase);

} // namespace net::minecraft::recipe
