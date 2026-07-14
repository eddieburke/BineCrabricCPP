#include "net/minecraft/item/armor/leather_boots.hpp"
#include <string>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/leather.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
LeatherBootsItem::LeatherBootsItem() : ArmorItem(kRawId, 0, 0, 3) {
}
void LeatherBootsItem::registerClass() {
  static LeatherBootsItem instance;
  instance.setTexturePosition(0, 3);
  instance.setTranslationKey("bootsCloth");
  Item::registerInItemsArray(&instance);
}
void LeatherBootsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(45)),
                                {std::string("X X"), std::string("X X"), 'X', Item::byRawId(78)});
}
MC_REGISTER_ITEM(LeatherBootsItem)
} // namespace net::minecraft::item
