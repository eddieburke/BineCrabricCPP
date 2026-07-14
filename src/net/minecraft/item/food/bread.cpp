#include "net/minecraft/item/food/bread.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/food/wheat.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
BreadItem::BreadItem() : FoodItem(kRawId, 5, false) {
}
void BreadItem::registerClass() {
  static BreadItem instance;
  instance.setTexturePosition(9, 2);
  instance.setTranslationKey("bread");
  Item::registerInItemsArray(&instance);
}
void BreadItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(41)), {std::string("###"), '#', Item::byRawId(40)});
}
MC_REGISTER_ITEM(BreadItem)
} // namespace net::minecraft::item
