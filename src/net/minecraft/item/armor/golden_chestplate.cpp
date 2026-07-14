#include "net/minecraft/item/armor/golden_chestplate.hpp"
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
GoldenChestplateItem::GoldenChestplateItem() : ArmorItem(kRawId, 1, 4, 1) {
}
void GoldenChestplateItem::registerClass() {
  static GoldenChestplateItem instance;
  instance.setTexturePosition(4, 1);
  instance.setTranslationKey("chestplateGold");
  Item::registerInItemsArray(&instance);
}
void GoldenChestplateItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(59)),
                                {std::string("X X"), std::string("XXX"), std::string("XXX"), 'X', Item::byRawId(10)});
}
MC_REGISTER_ITEM(GoldenChestplateItem)
} // namespace net::minecraft::item
