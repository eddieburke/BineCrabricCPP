#include "net/minecraft/item/weapon/wooden_sword.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

WoodenSwordItem::WoodenSwordItem() : SwordItem(kRawId, ToolMaterial::Wood) {}

void WoodenSwordItem::registerClass()
{
    static WoodenSwordItem instance;
    instance.setTexturePosition(0, 4);
    instance.setTranslationKey("swordWood");
    Item::registerInItemsArray(&instance);

}

void WoodenSwordItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(12)),
        {std::string("X"), std::string("X"), std::string("#"), '#', Item::byRawId(24), 'X', Block::PLANKS});
}

MC_REGISTER_ITEM(WoodenSwordItem)
} // namespace net::minecraft::item
