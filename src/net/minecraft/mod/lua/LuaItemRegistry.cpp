#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/lua/LuaItemModel.hpp"
#include "net/minecraft/mod/lua/ModIdRegistry.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include <string>
namespace net::minecraft::mod::lua {
namespace {
using net::minecraft::Item;
struct ItemTraits {
  using Spec = ItemRegistrationSpec;
  static constexpr const char* kKind = "item";
  static constexpr mod::LifecyclePhase kPhase = mod::LifecyclePhase::ItemRegistration;
  static void instantiate(const Spec& spec) {
    instantiateLuaModItem(spec);
  }
};
using ItemRegistry = ModIdRegistry<ItemTraits>;
} // namespace
bool registerItemSpec(const ItemRegistrationSpec& spec, std::string& error) {
  constexpr int kRawIdCount = Item::ITEM_COUNT - 256;
  const int rawId = spec.itemId - 256;
  if(spec.itemId < 256 || rawId >= kRawIdCount) {
    error = "register_item id must be between 256 and " + std::to_string(Item::ITEM_COUNT - 1);
    return false;
  }
  const bool hasModel = spec.model.type != LuaItemModelSpec::Type::Flat;
  if(spec.texturePath.empty() && spec.itemsTextureId < 0) {
    error = "register_item requires texture or texture_id";
    return false;
  }
  if(hasModel && spec.texturePath.empty()) {
    error = "register_item requires texture for a box_list or manual model";
    return false;
  }
  if(spec.itemsTextureId > 255) {
    error = "register_item texture_id must be a vanilla items-atlas index from 0 to 255";
    return false;
  }
  if(registry::Registry::isBootstrapped()) {
    error = "register_item must run while Lua mod scripts load at startup";
    return false;
  }
  if(ItemRegistry::instance().contains(spec.itemId)) {
    error = "register_item duplicate id: " + std::to_string(spec.itemId);
    return false;
  }
  if(!registry::Registry::tryReserveItemId(rawId)) {
    error = "register_item id is already reserved: " + std::to_string(spec.itemId);
    return false;
  }
  if(hasModel) {
    // Resolve (and cache) the packaged texture up front so a bad path fails
    // registration immediately instead of at first draw.
    (void)texture(spec.texturePath.c_str());
  }
  ItemRegistry::instance().add(spec.itemId, spec);
  return true;
}
int modItemIdFromName(const char* name) {
  return ItemRegistry::instance().idFromName(name);
}
std::string modItemWireName(int itemId) {
  return ItemRegistry::instance().wireName(itemId);
}
const ItemRegistrationSpec* itemRegistrationSpecForId(int itemId) noexcept {
  return ItemRegistry::instance().specForId(itemId);
}
} // namespace net::minecraft::mod::lua
