#pragma once
#include <string>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::mod::lua {
struct BlockRegistrationSpec {
 int blockId = 0;
 float hardness = 1.0f;
 float resistance = 1.0f;
 float luminance = 0.0f;
 std::string translationKey;
 std::string displayName;
 std::string material = "stone";
 std::string ownerModId;
 int bakedModel = 0; // model::ModelRegistry handle (from minecraft.model.load/build)
 bool opaque = true;
 bool fullCube = true;
  bool translucent = false;
 float collisionHeight = 1.0f;
 bool stackOnSame = false;
 bool requiresSolidBelow = true;
 bool coordinateBounds = false;
 bool coordinateColor = false;
 float boundsPadding = 0.0625f;
 float boundsOffset = 0.1f;
 float minScale = 0.9f;
 float maxScale = 1.1f;
 std::string itemTexturePath;
};
bool registerBlockSpec(const BlockRegistrationSpec& spec, std::string& error);
struct CoordinateVariedTransform {
 float scale = 1.0f;
 float offsetX = 0.0f;
 float offsetY = 0.0f;
 float offsetZ = 0.0f;
};
[[nodiscard]] CoordinateVariedTransform coordinateVariedTransform(
    const BlockRegistrationSpec& spec, int x, int y, int z);
[[nodiscard]] net::minecraft::Box coordinateVariedBlockBounds(const BlockRegistrationSpec& spec, int x, int y, int z);
[[nodiscard]] int modBlockIdFromName(const char* name);
[[nodiscard]] std::string modBlockWireName(int blockId);
[[nodiscard]] const BlockRegistrationSpec* blockRegistrationSpecForId(int blockId) noexcept;
} // namespace net::minecraft::mod::lua
