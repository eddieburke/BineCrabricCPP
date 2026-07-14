#include "net/minecraft/item/armor/golden_helmet.hpp"
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
GoldenHelmetItem::GoldenHelmetItem() : ArmorItem(kRawId, 1, 4, 0) {
}
void GoldenHelmetItem::registerClass() {
  static GoldenHelmetItem instance;
  instance.setTexturePosition(4, 0);
  instance.setTranslationKey("helmetGold");
  Item::registerInItemsArray(&instance);
}
void GoldenHelmetItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(58)),
                                {std::string("XXX"), std::string("X X"), 'X', Item::byRawId(10)});
}
MC_REGISTER_ITEM(GoldenHelmetItem)
} // namespace net::minecraft::item
