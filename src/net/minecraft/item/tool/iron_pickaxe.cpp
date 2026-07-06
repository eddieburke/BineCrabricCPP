#include "net/minecraft/item/tool/iron_pickaxe.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
namespace net::minecraft::item {
IronPickaxeItem::IronPickaxeItem() : PickaxeItem(kRawId, ToolMaterial::Iron) {}
void IronPickaxeItem::registerClass() {
  static IronPickaxeItem instance;
  instance.setTexturePosition(2, 6);
  instance.setTranslationKey("pickaxeIron");
  Item::registerInItemsArray(&instance);
}
void IronPickaxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(
      ItemStack(Item::byRawId(1)),
      {std::string("XXX"), std::string(" # "), std::string(" # "), '#', Item::byRawId(24), 'X', Item::byRawId(9)});
}
MC_REGISTER_ITEM(IronPickaxeItem)
} // namespace net::minecraft::item
