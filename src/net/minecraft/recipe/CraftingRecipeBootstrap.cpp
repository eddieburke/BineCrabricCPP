#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::recipe {
struct CraftingRecipeBootstrap {
  static void registerClass() {
    CraftingRecipeManager::registerVanillaRecipes();
  }
};
static registry::RegisterPhase<CraftingRecipeBootstrap> s_reg(mod::LifecyclePhase::CraftingRecipeRegistration, 0);
} // namespace net::minecraft::recipe
