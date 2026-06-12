#include "net/minecraft/item/armor/diamond_leggings.hpp"
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

DiamondLeggingsItem::DiamondLeggingsItem() : ArmorItem(kRawId, 3, 3, 2) {}

void DiamondLeggingsItem::registerClass()
{
    static DiamondLeggingsItem instance;
    instance.setTexturePosition(3, 2);
    instance.setTranslationKey("leggingsDiamond");
    Item::registerInItemsArray(&instance);

}

void DiamondLeggingsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(56)),
        {std::string("XXX"), std::string("X X"), std::string("X X"), 'X', Item::byRawId(8)});
}

static registry::RegisterItem<DiamondLeggingsItem> s_itemReg;
} // namespace net::minecraft::item
