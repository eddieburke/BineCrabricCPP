#include "net/minecraft/item/weapon/stone_sword.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
StoneSwordItem::StoneSwordItem() : SwordItem(kRawId, ToolMaterial::Stone) {
}
void StoneSwordItem::registerClass() {
  static StoneSwordItem instance;
  instance.setTexturePosition(1, 4);
  instance.setTranslationKey("swordStone");
  Item::registerInItemsArray(&instance);
}
void StoneSwordItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(
      ItemStack(Item::byRawId(16)),
      {std::string("X"), std::string("X"), std::string("#"), '#', Item::byRawId(24), 'X', Block::COBBLESTONE});
}
MC_REGISTER_ITEM(StoneSwordItem)
} // namespace net::minecraft::item
