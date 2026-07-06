#include "net/minecraft/item/weapon/golden_sword.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
namespace net::minecraft::item {
GoldenSwordItem::GoldenSwordItem() : SwordItem(kRawId, ToolMaterial::Gold) {}
void GoldenSwordItem::registerClass() {
  static GoldenSwordItem instance;
  instance.setTexturePosition(4, 4);
  instance.setTranslationKey("swordGold");
  Item::registerInItemsArray(&instance);
}
void GoldenSwordItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(27)), {std::string("X"), std::string("X"), std::string("#"),
                                                               '#', Item::byRawId(24), 'X', Item::byRawId(10)});
}
MC_REGISTER_ITEM(GoldenSwordItem)
} // namespace net::minecraft::item
