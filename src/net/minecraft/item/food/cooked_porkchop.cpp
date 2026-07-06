#include "net/minecraft/item/food/cooked_porkchop.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"
namespace net::minecraft::item {
CookedPorkchopItem::CookedPorkchopItem() : FoodItem(kRawId, 8, true) {}
void CookedPorkchopItem::registerClass() {
  static CookedPorkchopItem instance;
  instance.setTexturePosition(8, 5);
  instance.setTranslationKey("porkchopCooked");
  Item::registerInItemsArray(&instance);
}
void CookedPorkchopItem::registerSmeltingRecipes() {
  Item* raw = Item::byRawId(63);
  Item* cooked = Item::byRawId(64);
  if(raw != nullptr && cooked != nullptr) {
    recipe::SmeltingRecipeManager::instance().addRecipe(raw->id, ItemStack(cooked));
  }
}
void CookedPorkchopItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  (void)recipeManager;
}
MC_REGISTER_ITEM(CookedPorkchopItem)
} // namespace net::minecraft::item
