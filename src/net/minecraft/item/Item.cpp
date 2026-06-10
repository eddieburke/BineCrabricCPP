#include "net/minecraft/item/Item.hpp"

#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/client/resource/language/I18n.hpp"

#include <cassert>
#include <cstdio>

namespace net::minecraft {

std::array<Item*, Item::ITEM_COUNT> Item::ITEMS{};
JavaRandom Item::random{};

void Item::registerInItemsArray(Item* item)
{
    assert(item != nullptr);
    if (ITEMS[static_cast<std::size_t>(item->id)] != nullptr) {
        std::fprintf(stderr, "DUP item slot id=%d (rawId=%d)\n", item->id, item->id - 256);
    }
    assert(ITEMS[static_cast<std::size_t>(item->id)] == nullptr && "Item: duplicate slot registration");
    ITEMS[static_cast<std::size_t>(item->id)] = item;
}

std::string Item::getTranslatedName() const
{
    return client::resource::language::I18n::getTranslation(getTranslationKey() + ".name");
}

void initializeItems()
{
    // Items register via per-TU RegisterItem / RegisterCustom during Registry::bootstrap().
}

} // namespace net::minecraft
