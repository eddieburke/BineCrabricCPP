#include "net/minecraft/item/tool/stone_pickaxe.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
namespace net::minecraft::item {
StonePickaxeItem::StonePickaxeItem() : PickaxeItem(kRawId, ToolMaterial::Stone) {}
void StonePickaxeItem::registerClass() {
  static StonePickaxeItem instance;
  instance.setTexturePosition(1, 6);
  instance.setTranslationKey("pickaxeStone");
  Item::registerInItemsArray(&instance);
}
void StonePickaxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(
      ItemStack(Item::byRawId(18)),
      {std::string("XXX"), std::string(" # "), std::string(" # "), '#', Item::byRawId(24), 'X', Block::COBBLESTONE});
}
MC_REGISTER_ITEM(StonePickaxeItem)
} // namespace net::minecraft::item
