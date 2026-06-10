#include "net/minecraft/item/tool/iron_hoe.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

IronHoeItem::IronHoeItem() : HoeItem(36, ToolMaterial::Iron) {}

void IronHoeItem::registerClass()
{
    static IronHoeItem instance;
    instance.setTexturePosition(2, 8);
    instance.setTranslationKey("hoeIron");
    Item::registerInItemsArray(&instance);

}

void IronHoeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(36)),
        {std::string("XX"), std::string(" #"), std::string(" #"), '#', Item::byRawId(24), 'X', Item::byRawId(9)});
}

static registry::RegisterItem<IronHoeItem> s_itemReg(36);
} // namespace net::minecraft::item
