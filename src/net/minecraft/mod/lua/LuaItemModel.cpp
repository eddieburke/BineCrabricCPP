#include "net/minecraft/mod/lua/LuaItemModel.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/mod/lua/LuaModNaming.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include <algorithm>
#include <unordered_map>
#include <vector>
namespace net::minecraft::mod::lua {
namespace detail {
using net::minecraft::Item;
// Only exists to reach Item::setMaxDamage, which is protected; nothing else
// about a registered item needs subclass behavior (no world position, no
// per-instance virtual overrides).
class LuaModItem : public Item {
public:
  LuaModItem(int rawId, int maxDamage) : Item(rawId) {
    if(maxDamage > 0) {
      setMaxDamage(maxDamage);
    }
  }
};
void registerModItemDisplayName(const ItemRegistrationSpec& spec) {
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
}
void registerItemClass(const ItemRegistrationSpec& spec) {
  std::string translationKey = spec.translationKey;
  if(translationKey.empty()) {
    translationKey = "item" + std::to_string(spec.itemId);
  }
  const int textureId = !spec.texturePath.empty() ? texture(spec.texturePath.c_str()) : spec.itemsTextureId;
  registerModItemDisplayName(spec);
  const int rawId = spec.itemId - 256;
  (new LuaModItem(rawId, spec.maxDamage))->setTextureId(textureId)->setMaxCount(spec.maxCount)->setTranslationKey(translationKey);
}
} // namespace detail
void instantiateLuaModItem(const ItemRegistrationSpec& spec) {
  detail::registerItemClass(spec);
}
bool invokeManualItemModelDraw(const ItemRegistrationSpec& spec, float brightness) {
  if(spec.manualDrawRef == kLuaNoRef || spec.ownerModId.empty()) {
    return false;
  }
  LuaApi& api = luaApi();
  if(!api.ready()) {
    return false;
  }
  for(const std::shared_ptr<runtime::ModHost::LoadedLuaMod>& mod : runtime::host().loadedMods()) {
    if(mod == nullptr || mod->modId != spec.ownerModId) {
      continue;
    }
    const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
    if(!mod->active || mod->state == nullptr) {
      return false;
    }
    auto* state = static_cast<lua_State*>(mod->state);
    const int top = api.gettop(state);
    api.rawgeti(state, kLuaRegistryIndex, spec.manualDrawRef);
    if(api.type(state, -1) != kLuaTFunction) {
      api.settop(state, top);
      return false;
    }
    api.createtable(state, 0, 4);
    api.pushnumber(state, brightness);
    api.setfield(state, -2, "brightness");
    api.pushinteger(state, spec.itemId);
    api.setfield(state, -2, "item_id");
    api.pushstring(state, spec.texturePath.c_str());
    api.setfield(state, -2, "texture");
    api.pushinteger(state, spec.itemsTextureId);
    api.setfield(state, -2, "texture_id");
    const int status = api.pcallk(state, 1, 0, 0, 0, nullptr);
    api.settop(state, top);
    return status == kLuaOk;
  }
  return false;
}
__attribute__((weak)) bool drawLuaItemModel(client::render::Tessellator& /*tessellator*/, const ItemStack& /*stack*/,
                                            float /*brightness*/) {
  return false;
}
__attribute__((weak)) bool emitManualItemModelQuad(const ManualBlockVertex* /*vertices*/, int /*textureId*/,
                                                   float /*red*/, float /*green*/, float /*blue*/, float /*alpha*/) {
  return false;
}
namespace {
bool gItemModelHandOverride = false;
}
[[nodiscard]] bool itemModelHandOverrideActive() {
  return gItemModelHandOverride;
}
void setItemModelHandOverride(const bool enabled) {
  gItemModelHandOverride = enabled;
}
} // namespace net::minecraft::mod::lua
