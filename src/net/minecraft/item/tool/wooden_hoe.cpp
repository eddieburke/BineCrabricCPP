#include "net/minecraft/item/tool/wooden_hoe.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
WoodenHoeItem::WoodenHoeItem() : HoeItem(kRawId, ToolMaterial::Wood) {
}
void WoodenHoeItem::registerClass() {
 static WoodenHoeItem instance;
 instance.setTexturePosition(0, 8);
 instance.setTranslationKey("hoeWood");
 Item::registerInItemsArray(&instance);
}
void WoodenHoeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(
     ItemStack(Item::byRawId(34)),
     {std::string("XX"), std::string(" #"), std::string(" #"), '#', Item::byRawId(24), 'X', Block::PLANKS});
}
MC_REGISTER_ITEM(WoodenHoeItem)
} // namespace net::minecraft::item
