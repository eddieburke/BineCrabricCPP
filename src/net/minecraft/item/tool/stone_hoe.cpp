#include "net/minecraft/item/tool/stone_hoe.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

StoneHoeItem::StoneHoeItem() : HoeItem(kRawId, ToolMaterial::Stone) {}

void StoneHoeItem::registerClass()
{
    static StoneHoeItem instance;
    instance.setTexturePosition(1, 8);
    instance.setTranslationKey("hoeStone");
    Item::registerInItemsArray(&instance);

}

void StoneHoeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(35)),
        {std::string("XX"), std::string(" #"), std::string(" #"), '#', Item::byRawId(24), 'X', Block::COBBLESTONE});
}

static registry::RegisterItem<StoneHoeItem> s_itemReg;
} // namespace net::minecraft::item
