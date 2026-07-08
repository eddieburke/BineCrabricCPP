#include "net/minecraft/item/misc/paper.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
PaperItem::PaperItem() : Item(kRawId, RegistrationMode::Deferred) {
}

void PaperItem::registerClass() {
    static PaperItem instance;
    instance.setTexturePosition(10, 3);
    instance.setTranslationKey("paper");
    Item::registerInItemsArray(&instance);
}

void PaperItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(83), 3), {std::string("###"), '#', Item::byRawId(82)});
}
MC_REGISTER_ITEM(PaperItem)
}  // namespace net::minecraft::item
