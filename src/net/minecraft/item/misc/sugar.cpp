#include "net/minecraft/item/misc/sugar.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::item {

SugarItem::SugarItem() : Item(97, RegistrationMode::Deferred) {}

void SugarItem::registerClass()
{
    static SugarItem instance;
    instance.setTexturePosition(13, 0);
    instance.setHandheld();
    instance.setTranslationKey("sugar");
    Item::registerInItemsArray(&instance);

}

void SugarItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(97)),
        {std::string("#"), '#', Item::byRawId(82)});
}

static registry::RegisterItem<SugarItem> s_itemReg(97);
} // namespace net::minecraft::item
