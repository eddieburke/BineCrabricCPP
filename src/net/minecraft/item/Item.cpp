#include "net/minecraft/item/Item.hpp"
#include <cassert>
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft {
std::array<Item*, Item::ITEM_COUNT> Item::ITEMS{};
JavaRandom Item::random{};
void Item::registerInItemsArray(Item* item) {
  assert(item != nullptr);
  assert(ITEMS[static_cast<std::size_t>(item->id)] == nullptr && "Item: duplicate slot registration");
  ITEMS[static_cast<std::size_t>(item->id)] = item;
}
std::string Item::getTranslatedName() const {
  return client::resource::language::I18n::getTranslation(getTranslationKey() + ".name");
}
} // namespace net::minecraft
