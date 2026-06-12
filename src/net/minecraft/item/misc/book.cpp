#include "net/minecraft/item/misc/book.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

BookItem::BookItem() : Item(kRawId, RegistrationMode::Deferred) {}

void BookItem::registerClass()
{
    static BookItem instance;
    instance.setTexturePosition(11, 3);
    instance.setTranslationKey("book");
    Item::registerInItemsArray(&instance);

}

void BookItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(84), 1),
        {std::string("#"), std::string("#"), std::string("#"), '#', Item::byRawId(83)});
}

static registry::RegisterItem<BookItem> s_itemReg;
} // namespace net::minecraft::item
