#include "net/minecraft/item/tool/iron_axe.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

IronAxeItem::IronAxeItem() : AxeItem(2, ToolMaterial::Iron) {}

void IronAxeItem::registerClass()
{
    static IronAxeItem instance;
    instance.setTexturePosition(2, 7);
    instance.setTranslationKey("hatchetIron");
    Item::registerInItemsArray(&instance);

}

void IronAxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(2)),
        {std::string("XX"), std::string("X#"), std::string(" #"), '#', Item::byRawId(24), 'X', Item::byRawId(9)});
}

static registry::RegisterItem<IronAxeItem> s_itemReg(2);
} // namespace net::minecraft::item
