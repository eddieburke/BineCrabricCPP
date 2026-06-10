#include "net/minecraft/item/armor/chain_chestplate.hpp"

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

ChainChestplateItem::ChainChestplateItem() : ArmorItem(47, 1, 1, 1) {}

void ChainChestplateItem::registerClass()
{
    static ChainChestplateItem instance;
    instance.setTexturePosition(1, 1);
    instance.setTranslationKey("chestplateChain");
    Item::registerInItemsArray(&instance);

}

void ChainChestplateItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(47)),
        {std::string("X X"), std::string("XXX"), std::string("XXX"), 'X', Block::FIRE});
}

static registry::RegisterItem<ChainChestplateItem> s_itemReg(47);
} // namespace net::minecraft::item
