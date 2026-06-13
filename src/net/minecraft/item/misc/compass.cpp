#include "net/minecraft/item/misc/compass.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::item {

CompassItem::CompassItem() : Item(kRawId, RegistrationMode::Deferred) {}

void CompassItem::registerClass()
{
    static CompassItem instance;
    instance.setTexturePosition(6, 3);
    instance.setTranslationKey("compass");
    Item::registerInItemsArray(&instance);

}

void CompassItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(89)),
        {std::string(" # "), std::string("#X#"), std::string(" # "), '#', Item::byRawId(9), 'X', Item::byRawId(75)});
}

MC_REGISTER_ITEM(CompassItem)
} // namespace net::minecraft::item
