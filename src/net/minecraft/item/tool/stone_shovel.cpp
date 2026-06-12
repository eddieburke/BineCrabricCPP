#include "net/minecraft/item/tool/stone_shovel.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

StoneShovelItem::StoneShovelItem() : ShovelItem(kRawId, ToolMaterial::Stone) {}

void StoneShovelItem::registerClass()
{
    static StoneShovelItem instance;
    instance.setTexturePosition(1, 5);
    instance.setTranslationKey("shovelStone");
    Item::registerInItemsArray(&instance);

}

void StoneShovelItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(17)),
        {std::string("X"), std::string("#"), std::string("#"), '#', Item::byRawId(24), 'X', Block::COBBLESTONE});
}

static registry::RegisterItem<StoneShovelItem> s_itemReg;
} // namespace net::minecraft::item
