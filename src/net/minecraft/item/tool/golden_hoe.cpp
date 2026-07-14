#include "net/minecraft/item/tool/golden_hoe.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
GoldenHoeItem::GoldenHoeItem() : HoeItem(kRawId, ToolMaterial::Gold) {
}
void GoldenHoeItem::registerClass() {
  static GoldenHoeItem instance;
  instance.setTexturePosition(4, 8);
  instance.setTranslationKey("hoeGold");
  Item::registerInItemsArray(&instance);
}
void GoldenHoeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(
      ItemStack(Item::byRawId(38)),
      {std::string("XX"), std::string(" #"), std::string(" #"), '#', Item::byRawId(24), 'X', Item::byRawId(10)});
}
MC_REGISTER_ITEM(GoldenHoeItem)
} // namespace net::minecraft::item
