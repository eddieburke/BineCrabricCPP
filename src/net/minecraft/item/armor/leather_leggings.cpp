#include "net/minecraft/item/armor/leather_leggings.hpp"
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
LeatherLeggingsItem::LeatherLeggingsItem() : ArmorItem(kRawId, 0, 0, 2) {}
void LeatherLeggingsItem::registerClass() {
  static LeatherLeggingsItem instance;
  instance.setTexturePosition(0, 2);
  instance.setTranslationKey("leggingsCloth");
  Item::registerInItemsArray(&instance);
}
void LeatherLeggingsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(44)),
                                {std::string("XXX"), std::string("X X"), std::string("X X"), 'X', Item::byRawId(78)});
}
MC_REGISTER_ITEM(LeatherLeggingsItem)
} // namespace net::minecraft::item
