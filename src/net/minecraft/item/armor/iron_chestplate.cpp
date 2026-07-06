#include "net/minecraft/item/armor/iron_chestplate.hpp"
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
IronChestplateItem::IronChestplateItem() : ArmorItem(kRawId, 2, 2, 1) {}
void IronChestplateItem::registerClass() {
  static IronChestplateItem instance;
  instance.setTexturePosition(2, 1);
  instance.setTranslationKey("chestplateIron");
  Item::registerInItemsArray(&instance);
}
void IronChestplateItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(51)),
                                {std::string("X X"), std::string("XXX"), std::string("XXX"), 'X', Item::byRawId(9)});
}
MC_REGISTER_ITEM(IronChestplateItem)
} // namespace net::minecraft::item
