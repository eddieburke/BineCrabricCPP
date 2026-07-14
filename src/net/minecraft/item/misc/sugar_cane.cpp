#include "net/minecraft/item/misc/sugar_cane.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
SugarCaneItem::SugarCaneItem() : SecondaryBlockItem(kRawId, Block::SUGAR_CANE) {
}
void SugarCaneItem::registerClass() {
  static SugarCaneItem instance;
  instance.setTexturePosition(11, 1);
  instance.setTranslationKey("reeds");
  Item::registerInItemsArray(&instance);
}
void SugarCaneItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  (void)recipeManager;
}
MC_REGISTER_ITEM(SugarCaneItem)
} // namespace net::minecraft::item
