#pragma once
#include "net/minecraft/mod/lua/LuaBlockModel.hpp"
#include <string>
namespace net::minecraft::mod::lua {
struct BlockRegistrationSpec {
  int blockId = 0;
  std::string texturePath;
  int terrainTextureId = -1;
  float hardness = 1.0f;
  float resistance = 1.0f;
  float luminance = 0.0f;
  std::string translationKey;
  std::string displayName;
  std::string material = "stone";
  std::string ownerModId;
  int manualDrawRef = -2;
  int manualInventoryRef = -2;
  LuaBlockModelSpec model;
  std::string itemTexturePath;
  int itemTextureId = -1;
};
bool registerBlockSpec(const BlockRegistrationSpec& spec, std::string& error);
[[nodiscard]] int modBlockIdFromName(const char* name);
[[nodiscard]] std::string modBlockWireName(int blockId);
[[nodiscard]] const BlockRegistrationSpec* blockRegistrationSpecForId(int blockId) noexcept;
} // namespace net::minecraft::mod::lua
