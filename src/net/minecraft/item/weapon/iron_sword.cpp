#include "net/minecraft/item/weapon/iron_sword.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
IronSwordItem::IronSwordItem() : SwordItem(kRawId, ToolMaterial::Iron) {
}
void IronSwordItem::registerClass() {
  static IronSwordItem instance;
  instance.setTexturePosition(2, 4);
  instance.setTranslationKey("swordIron");
  Item::registerInItemsArray(&instance);
}
void IronSwordItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(
      ItemStack(Item::byRawId(11)),
      {std::string("X"), std::string("X"), std::string("#"), '#', Item::byRawId(24), 'X', Item::byRawId(9)});
}
MC_REGISTER_ITEM(IronSwordItem)
} // namespace net::minecraft::item
