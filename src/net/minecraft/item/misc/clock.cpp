#include "net/minecraft/item/misc/clock.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::item {

ClockItem::ClockItem() : Item(kRawId, RegistrationMode::Deferred) {}

void ClockItem::registerClass()
{
    static ClockItem instance;
    instance.setTexturePosition(6, 4);
    instance.setTranslationKey("clock");
    Item::registerInItemsArray(&instance);

}

void ClockItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(91)),
        {std::string(" # "), std::string("#X#"), std::string(" # "), '#', Item::byRawId(10), 'X', Item::byRawId(75)});
}

static registry::RegisterItem<ClockItem> s_itemReg;
} // namespace net::minecraft::item
