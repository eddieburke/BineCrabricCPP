#include "net/minecraft/item/armor/chain_boots.hpp"

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

ChainBootsItem::ChainBootsItem() : ArmorItem(49, 1, 1, 3) {}

void ChainBootsItem::registerClass()
{
    static ChainBootsItem instance;
    instance.setTexturePosition(1, 3);
    instance.setTranslationKey("bootsChain");
    Item::registerInItemsArray(&instance);

}

void ChainBootsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(49)),
        {std::string("X X"), std::string("X X"), 'X', Block::FIRE});
}

static registry::RegisterItem<ChainBootsItem> s_itemReg(49);
} // namespace net::minecraft::item
