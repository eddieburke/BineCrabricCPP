#pragma once
#include <string>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
namespace net::minecraft::mod::lua {
struct ItemRegistrationSpec {
  int itemId = 0; // Absolute id (256 + raw id), matching ItemStack.itemId / minecraft.items.ids.
  std::string texturePath;
  int itemsTextureId = -1; // Vanilla /gui/items.png atlas index (0..255), alternative to texturePath.
  int maxCount = 64;
  int maxDamage = 0;
  std::string translationKey;
  std::string displayName;
  std::string ownerModId;
  int modelRef = kLuaNoRef;
  int bakedModel = 0; // model::ModelRegistry handle; preferred over modelRef
};
bool registerItemSpec(const ItemRegistrationSpec& spec, std::string& error);
[[nodiscard]] int modItemIdFromName(const char* name);
[[nodiscard]] std::string modItemWireName(int itemId);
[[nodiscard]] const ItemRegistrationSpec* itemRegistrationSpecForId(int itemId) noexcept;
[[nodiscard]] bool itemModelHandOverrideActive();
void setItemModelHandOverride(bool enabled);
} // namespace net::minecraft::mod::lua
