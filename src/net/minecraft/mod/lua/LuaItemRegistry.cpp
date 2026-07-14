#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/resource/language/I18n.hpp"
#endif
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaModNaming.hpp"
#include "net/minecraft/mod/lua/ModIdRegistry.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::mod::lua {
namespace {
using net::minecraft::Item;
class LuaModItem : public Item {
public:
  LuaModItem(int rawId, int maxDamage) : Item(rawId) {
    if(maxDamage > 0) {
      setMaxDamage(maxDamage);
    }
  }
};
void registerModItemDisplayName(const ItemRegistrationSpec& spec) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  std::string keyToken = spec.translationKey;
  if(keyToken.empty()) {
    keyToken = "item" + std::to_string(spec.itemId);
  }
  const std::string i18nKey = "item." + keyToken + ".name";
  std::string label = spec.displayName;
  if(label.empty()) {
    label = humanizeTranslationKey(keyToken);
  }
  if(label.empty()) {
    label = "Item " + std::to_string(spec.itemId);
  }
  client::resource::language::I18n::addTranslation(i18nKey, std::move(label));
#else
  (void)spec;
#endif
}
void registerItemClass(const ItemRegistrationSpec& spec) {
  std::string translationKey = spec.translationKey;
  if(translationKey.empty()) {
    translationKey = "item" + std::to_string(spec.itemId);
  }
  const int textureId = !spec.texturePath.empty() ? texture(spec.texturePath.c_str()) : spec.itemsTextureId;
  registerModItemDisplayName(spec);
  const int rawId = spec.itemId - 256;
  (new LuaModItem(rawId, spec.maxDamage))
      ->setTextureId(textureId)
      ->setMaxCount(spec.maxCount)
      ->setTranslationKey(translationKey);
}
void instantiateLuaModItem(const ItemRegistrationSpec& spec) {
  registerItemClass(spec);
}
struct ItemTraits {
  using Spec = ItemRegistrationSpec;
  static constexpr const char* kKind = "item";
  static constexpr mod::LifecyclePhase kPhase = mod::LifecyclePhase::Init;
  static void instantiate(const Spec& spec) {
    instantiateLuaModItem(spec);
  }
};
using ItemRegistry = ModIdRegistry<ItemTraits>;
bool gItemModelHandOverride = false;
} // namespace
[[nodiscard]] bool itemModelHandOverrideActive() {
  return gItemModelHandOverride;
}
void setItemModelHandOverride(const bool enabled) {
  gItemModelHandOverride = enabled;
}
bool registerItemSpec(const ItemRegistrationSpec& spec, std::string& error) {
  constexpr int kRawIdCount = Item::ITEM_COUNT - 256;
  const int rawId = spec.itemId - 256;
  if(spec.itemId < 256 || rawId >= kRawIdCount) {
    error = "register_item id must be between 256 and " + std::to_string(Item::ITEM_COUNT - 1);
    return false;
  }
  const bool hasModel = spec.modelRef != kLuaNoRef;
  if(spec.texturePath.empty() && spec.itemsTextureId < 0) {
    error = "register_item requires texture or texture_id";
    return false;
  }
  if(hasModel && spec.texturePath.empty()) {
    error = "register_item requires texture for a model";
    return false;
  }
  if(spec.itemsTextureId > 255) {
    error = "register_item texture_id must be a vanilla items-atlas index from 0 to 255";
    return false;
  }
  if(mod::ModLifecycle::currentPhase() == mod::LifecyclePhase::PostInit ||
     mod::ModLifecycle::currentPhase() == mod::LifecyclePhase::Ready) {
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
