#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::recipe {

struct CraftingRecipeFinalizer {
    static void registerClass() { CraftingRecipeManager::getInstance().finishRegistration(); }
};

static registry::RegisterCustom<CraftingRecipeFinalizer> s_reg(registry::kCraftingRecipeFinalizePriority);

} // namespace net::minecraft::recipe
