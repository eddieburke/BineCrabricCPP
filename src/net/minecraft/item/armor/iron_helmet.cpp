#include "net/minecraft/item/armor/iron_helmet.hpp"
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
IronHelmetItem::IronHelmetItem() : ArmorItem(kRawId, 2, 2, 0) {
}
void IronHelmetItem::registerClass() {
  static IronHelmetItem instance;
  instance.setTexturePosition(2, 0);
  instance.setTranslationKey("helmetIron");
  Item::registerInItemsArray(&instance);
}
void IronHelmetItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(50)),
                                {std::string("XXX"), std::string("X X"), 'X', Item::byRawId(9)});
}
MC_REGISTER_ITEM(IronHelmetItem)
} // namespace net::minecraft::item
