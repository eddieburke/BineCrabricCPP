#include "net/minecraft/item/tool/diamond_shovel.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

DiamondShovelItem::DiamondShovelItem() : ShovelItem(21, ToolMaterial::Diamond) {}

void DiamondShovelItem::registerClass()
{
    static DiamondShovelItem instance;
    instance.setTexturePosition(3, 5);
    instance.setTranslationKey("shovelDiamond");
    Item::registerInItemsArray(&instance);

}

void DiamondShovelItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(21)),
        {std::string("X"), std::string("#"), std::string("#"), '#', Item::byRawId(24), 'X', Item::byRawId(8)});
}

static registry::RegisterItem<DiamondShovelItem> s_itemReg(21);
} // namespace net::minecraft::item
