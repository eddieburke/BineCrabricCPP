#include "net/minecraft/item/armor/leather_helmet.hpp"

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

LeatherHelmetItem::LeatherHelmetItem() : ArmorItem(42, 0, 0, 0) {}

void LeatherHelmetItem::registerClass()
{
    static LeatherHelmetItem instance;
    instance.setTexturePosition(0, 0);
    instance.setTranslationKey("helmetCloth");
    Item::registerInItemsArray(&instance);

}

void LeatherHelmetItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(42)),
        {std::string("XXX"), std::string("X X"), 'X', Item::byRawId(78)});
}

static registry::RegisterItem<LeatherHelmetItem> s_itemReg(42);
} // namespace net::minecraft::item
