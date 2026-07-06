#include "net/minecraft/item/armor/diamond_chestplate.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include <string>
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/leather.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
namespace net::minecraft::item {
DiamondChestplateItem::DiamondChestplateItem() : ArmorItem(kRawId, 3, 3, 1) {}
void DiamondChestplateItem::registerClass() {
  static DiamondChestplateItem instance;
  instance.setTexturePosition(3, 1);
  instance.setTranslationKey("chestplateDiamond");
  Item::registerInItemsArray(&instance);
}
void DiamondChestplateItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(55)),
                                {std::string("X X"), std::string("XXX"), std::string("XXX"), 'X', Item::byRawId(8)});
}
MC_REGISTER_ITEM(DiamondChestplateItem)
} // namespace net::minecraft::item
