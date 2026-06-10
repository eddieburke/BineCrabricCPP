#include "net/minecraft/item/misc/paper.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

PaperItem::PaperItem() : Item(83, RegistrationMode::Deferred) {}

void PaperItem::registerClass()
{
    static PaperItem instance;
    instance.setTexturePosition(10, 3);
    instance.setTranslationKey("paper");
    Item::registerInItemsArray(&instance);

}

void PaperItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(83), 3), {std::string("###"), '#', Item::byRawId(82)});
}

static registry::RegisterItem<PaperItem> s_itemReg(83);
} // namespace net::minecraft::item
