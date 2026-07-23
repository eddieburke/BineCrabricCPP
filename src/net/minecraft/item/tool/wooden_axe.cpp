#include "net/minecraft/item/tool/wooden_axe.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
WoodenAxeItem::WoodenAxeItem() : AxeItem(kRawId, ToolMaterial::Wood) {
}
void WoodenAxeItem::registerClass() {
 static WoodenAxeItem instance;
 instance.setTexturePosition(0, 7);
 instance.setTranslationKey("hatchetWood");
 Item::registerInItemsArray(&instance);
}
void WoodenAxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(
     ItemStack(Item::byRawId(15)),
     {std::string("XX"), std::string("X#"), std::string(" #"), '#', Item::byRawId(24), 'X', Block::PLANKS});
}
MC_REGISTER_ITEM(WoodenAxeItem)
} // namespace net::minecraft::item
