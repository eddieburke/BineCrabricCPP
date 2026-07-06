#include "net/minecraft/item/tool/stone_axe.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
namespace net::minecraft::item {
StoneAxeItem::StoneAxeItem() : AxeItem(kRawId, ToolMaterial::Stone) {}
void StoneAxeItem::registerClass() {
  static StoneAxeItem instance;
  instance.setTexturePosition(1, 7);
  instance.setTranslationKey("hatchetStone");
  Item::registerInItemsArray(&instance);
}
void StoneAxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(
      ItemStack(Item::byRawId(19)),
      {std::string("XX"), std::string("X#"), std::string(" #"), '#', Item::byRawId(24), 'X', Block::COBBLESTONE});
}
MC_REGISTER_ITEM(StoneAxeItem)
} // namespace net::minecraft::item
