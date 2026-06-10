#include "net/minecraft/item/armor/chain_helmet.hpp"

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

ChainHelmetItem::ChainHelmetItem() : ArmorItem(46, 1, 1, 0) {}

void ChainHelmetItem::registerClass()
{
    static ChainHelmetItem instance;
    instance.setTexturePosition(1, 0);
    instance.setTranslationKey("helmetChain");
    Item::registerInItemsArray(&instance);

}

void ChainHelmetItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(46)),
        {std::string("XXX"), std::string("X X"), 'X', Block::FIRE});
}

static registry::RegisterItem<ChainHelmetItem> s_itemReg(46);
} // namespace net::minecraft::item
