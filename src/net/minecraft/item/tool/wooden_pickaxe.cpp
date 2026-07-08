#include "net/minecraft/item/tool/wooden_pickaxe.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
WoodenPickaxeItem::WoodenPickaxeItem() : PickaxeItem(kRawId, ToolMaterial::Wood) {
}

void WoodenPickaxeItem::registerClass() {
    static WoodenPickaxeItem instance;
    instance.setTexturePosition(0, 6);
    instance.setTranslationKey("pickaxeWood");
    Item::registerInItemsArray(&instance);
}

void WoodenPickaxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(
        ItemStack(Item::byRawId(14)),
        {std::string("XXX"), std::string(" # "), std::string(" # "), '#', Item::byRawId(24), 'X', Block::PLANKS});
}
MC_REGISTER_ITEM(WoodenPickaxeItem)
}  // namespace net::minecraft::item
