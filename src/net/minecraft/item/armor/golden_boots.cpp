#include "net/minecraft/item/armor/golden_boots.hpp"
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
GoldenBootsItem::GoldenBootsItem() : ArmorItem(kRawId, 1, 4, 3) {
}
void GoldenBootsItem::registerClass() {
  static GoldenBootsItem instance;
  instance.setTexturePosition(4, 3);
  instance.setTranslationKey("bootsGold");
  Item::registerInItemsArray(&instance);
}
void GoldenBootsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(61)),
                                {std::string("X X"), std::string("X X"), 'X', Item::byRawId(10)});
}
MC_REGISTER_ITEM(GoldenBootsItem)
} // namespace net::minecraft::item
