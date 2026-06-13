#include "net/minecraft/item/tool/diamond_axe.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

DiamondAxeItem::DiamondAxeItem() : AxeItem(kRawId, ToolMaterial::Diamond) {}

void DiamondAxeItem::registerClass()
{
    static DiamondAxeItem instance;
    instance.setTexturePosition(3, 7);
    instance.setTranslationKey("hatchetDiamond");
    Item::registerInItemsArray(&instance);

}

void DiamondAxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(23)),
        {std::string("XX"), std::string("X#"), std::string(" #"), '#', Item::byRawId(24), 'X', Item::byRawId(8)});
}

MC_REGISTER_ITEM(DiamondAxeItem)
} // namespace net::minecraft::item
