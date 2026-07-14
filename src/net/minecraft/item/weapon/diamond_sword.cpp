#include "net/minecraft/item/weapon/diamond_sword.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
DiamondSwordItem::DiamondSwordItem() : SwordItem(kRawId, ToolMaterial::Diamond) {
}
void DiamondSwordItem::registerClass() {
  static DiamondSwordItem instance;
  instance.setTexturePosition(3, 4);
  instance.setTranslationKey("swordDiamond");
  Item::registerInItemsArray(&instance);
}
void DiamondSwordItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(
      ItemStack(Item::byRawId(20)),
      {std::string("X"), std::string("X"), std::string("#"), '#', Item::byRawId(24), 'X', Item::byRawId(8)});
}
MC_REGISTER_ITEM(DiamondSwordItem)
} // namespace net::minecraft::item
