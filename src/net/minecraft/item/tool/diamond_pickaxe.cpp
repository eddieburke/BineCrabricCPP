#include "net/minecraft/item/tool/diamond_pickaxe.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

DiamondPickaxeItem::DiamondPickaxeItem() : PickaxeItem(kRawId, ToolMaterial::Diamond) {}

void DiamondPickaxeItem::registerClass()
{
    static DiamondPickaxeItem instance;
    instance.setTexturePosition(3, 6);
    instance.setTranslationKey("pickaxeDiamond");
    Item::registerInItemsArray(&instance);

}

void DiamondPickaxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(22)),
        {std::string("XXX"), std::string(" # "), std::string(" # "), '#', Item::byRawId(24), 'X', Item::byRawId(8)});
}

static registry::RegisterItem<DiamondPickaxeItem> s_itemReg;
} // namespace net::minecraft::item
