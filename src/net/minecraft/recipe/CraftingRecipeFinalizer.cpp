#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::recipe {
struct CraftingRecipeFinalizer {
  static void registerClass() {
    CraftingRecipeManager::getInstance().finishRegistration();
  }
};
static registry::RegisterPhase<CraftingRecipeFinalizer> s_reg(mod::LifecyclePhase::CraftingRecipeRegistration,
                                                              registry::kCraftingRecipeFinalizeOrder);
} // namespace net::minecraft::recipe
