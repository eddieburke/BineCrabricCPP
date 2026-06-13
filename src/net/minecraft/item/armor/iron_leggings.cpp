#include "net/minecraft/item/armor/iron_leggings.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/leather.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

IronLeggingsItem::IronLeggingsItem() : ArmorItem(kRawId, 2, 2, 2) {}

void IronLeggingsItem::registerClass()
{
    static IronLeggingsItem instance;
    instance.setTexturePosition(2, 2);
    instance.setTranslationKey("leggingsIron");
    Item::registerInItemsArray(&instance);

}

void IronLeggingsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(52)),
        {std::string("XXX"), std::string("X X"), std::string("X X"), 'X', Item::byRawId(9)});
}

MC_REGISTER_ITEM(IronLeggingsItem)
} // namespace net::minecraft::item
