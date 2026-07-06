#include "net/minecraft/item/armor/leather_chestplate.hpp"
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
LeatherChestplateItem::LeatherChestplateItem() : ArmorItem(kRawId, 0, 0, 1) {}
void LeatherChestplateItem::registerClass() {
  static LeatherChestplateItem instance;
  instance.setTexturePosition(0, 1);
  instance.setTranslationKey("chestplateCloth");
  Item::registerInItemsArray(&instance);
}
void LeatherChestplateItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(43)),
                                {std::string("X X"), std::string("XXX"), std::string("XXX"), 'X', Item::byRawId(78)});
}
MC_REGISTER_ITEM(LeatherChestplateItem)
} // namespace net::minecraft::item
