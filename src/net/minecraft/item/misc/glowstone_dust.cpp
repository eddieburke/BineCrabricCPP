#include "net/minecraft/item/misc/glowstone_dust.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

GlowstoneDustItem::GlowstoneDustItem() : Item(kRawId, RegistrationMode::Deferred) {}

void GlowstoneDustItem::registerClass()
{
    static GlowstoneDustItem instance;
    instance.setTexturePosition(9, 4);
    instance.setTranslationKey("yellowDust");
    Item::registerInItemsArray(&instance);

}

void GlowstoneDustItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<GlowstoneDustItem> s_itemReg;
} // namespace net::minecraft::item
