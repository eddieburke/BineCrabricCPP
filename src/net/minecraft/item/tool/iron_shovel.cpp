#include "net/minecraft/item/tool/iron_shovel.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

IronShovelItem::IronShovelItem() : ShovelItem(0, ToolMaterial::Iron) {}

void IronShovelItem::registerClass()
{
    static IronShovelItem instance;
    instance.setTexturePosition(2, 5);
    instance.setTranslationKey("shovelIron");
    Item::registerInItemsArray(&instance);

}

void IronShovelItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(0)),
        {std::string("X"), std::string("#"), std::string("#"), '#', Item::byRawId(24), 'X', Item::byRawId(9)});
}

static registry::RegisterItem<IronShovelItem> s_itemReg(0);
} // namespace net::minecraft::item
