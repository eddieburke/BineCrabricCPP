#include "net/minecraft/item/armor/diamond_helmet.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/leather.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

DiamondHelmetItem::DiamondHelmetItem() : ArmorItem(54, 3, 3, 0) {}

void DiamondHelmetItem::registerClass()
{
    static DiamondHelmetItem instance;
    instance.setTexturePosition(3, 0);
    instance.setTranslationKey("helmetDiamond");
    Item::registerInItemsArray(&instance);

}

void DiamondHelmetItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(54)),
        {std::string("XXX"), std::string("X X"), 'X', Item::byRawId(8)});
}

static registry::RegisterItem<DiamondHelmetItem> s_itemReg(54);
} // namespace net::minecraft::item
