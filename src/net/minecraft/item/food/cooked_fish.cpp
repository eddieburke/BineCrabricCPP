#include "net/minecraft/item/food/cooked_fish.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
CookedFishItem::CookedFishItem() : FoodItem(kRawId, 5, false) {
}
void CookedFishItem::registerClass() {
  static CookedFishItem instance;
  instance.setTexturePosition(10, 5);
  instance.setTranslationKey("fishCooked");
  Item::registerInItemsArray(&instance);
}
void CookedFishItem::registerSmeltingRecipes() {
  Item* raw = Item::byRawId(93);
  Item* cooked = Item::byRawId(94);
  if(raw != nullptr && cooked != nullptr) {
    recipe::SmeltingRecipeManager::instance().addRecipe(raw->id, ItemStack(cooked));
  }
}
void CookedFishItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  (void)recipeManager;
}
MC_REGISTER_ITEM(CookedFishItem)
} // namespace net::minecraft::item
