#include "net/minecraft/item/armor/chain_leggings.hpp"

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

ChainLeggingsItem::ChainLeggingsItem() : ArmorItem(48, 1, 1, 2) {}

void ChainLeggingsItem::registerClass()
{
    static ChainLeggingsItem instance;
    instance.setTexturePosition(1, 2);
    instance.setTranslationKey("leggingsChain");
    Item::registerInItemsArray(&instance);

}

void ChainLeggingsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(48)),
        {std::string("XXX"), std::string("X X"), std::string("X X"), 'X', Block::FIRE});
}

static registry::RegisterItem<ChainLeggingsItem> s_itemReg(48);
} // namespace net::minecraft::item
