#include "net/minecraft/item/tool/diamond_hoe.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

DiamondHoeItem::DiamondHoeItem() : HoeItem(37, ToolMaterial::Diamond) {}

void DiamondHoeItem::registerClass()
{
    static DiamondHoeItem instance;
    instance.setTexturePosition(3, 8);
    instance.setTranslationKey("hoeDiamond");
    Item::registerInItemsArray(&instance);

}

void DiamondHoeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(37)),
        {std::string("XX"), std::string(" #"), std::string(" #"), '#', Item::byRawId(24), 'X', Item::byRawId(8)});
}

static registry::RegisterItem<DiamondHoeItem> s_itemReg(37);
} // namespace net::minecraft::item
