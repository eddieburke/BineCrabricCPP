#include "net/minecraft/item/armor/golden_leggings.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/leather.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

GoldenLeggingsItem::GoldenLeggingsItem() : ArmorItem(kRawId, 1, 4, 2) {}

void GoldenLeggingsItem::registerClass()
{
    static GoldenLeggingsItem instance;
    instance.setTexturePosition(4, 2);
    instance.setTranslationKey("leggingsGold");
    Item::registerInItemsArray(&instance);

}

void GoldenLeggingsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(60)),
        {std::string("XXX"), std::string("X X"), std::string("X X"), 'X', Item::byRawId(10)});
}

static registry::RegisterItem<GoldenLeggingsItem> s_itemReg;
} // namespace net::minecraft::item
