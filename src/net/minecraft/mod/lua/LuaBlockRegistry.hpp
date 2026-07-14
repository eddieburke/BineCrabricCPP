#pragma once
#include <string>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/util/math/Types.hpp"
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
  int modelRef = kLuaNoRef;
  int bakedModel = 0; // model::ModelRegistry handle; preferred over modelRef
  bool opaque = true;
  bool fullCube = true;
  // Render layer: solid/cutout (false) or alpha-blended (true). Defaults to
  // !opaque at parse time for backward compatibility; solid-looking non-opaque
  // blocks should pass translucent=false so they draw in the solid pass (and
  // cast shader shadows, which skip the translucent layer).
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
  int itemTextureId = -1;
  std::string tileEntityId; // non-empty => block owns a mod tile entity (registry id = ownerModId + ":" + this)
};
bool registerBlockSpec(const BlockRegistrationSpec& spec, std::string& error);
// Raw per-block-coordinate scale/offset (built on util::math::coordinateRandom,
// no padding applied). Shared by coordinateVariedBlockBounds (vanilla-shaped
// render path) and baked-model quad rendering, so a block's collision/render
// box and its custom model jitter by the same amount at the same position.
struct CoordinateVariedTransform {
  float scale = 1.0f;
  float offsetX = 0.0f;
  float offsetY = 0.0f;
  float offsetZ = 0.0f;
};
[[nodiscard]] CoordinateVariedTransform coordinateVariedTransform(
    const BlockRegistrationSpec& spec, int x, int y, int z);
// Padded render/collision box derived from coordinateVariedTransform, used by
// the vanilla-shaped render path (getRenderBounds).
[[nodiscard]] net::minecraft::Box coordinateVariedBlockBounds(const BlockRegistrationSpec& spec, int x, int y, int z);
[[nodiscard]] int modBlockIdFromName(const char* name);
[[nodiscard]] std::string modBlockWireName(int blockId);
[[nodiscard]] const BlockRegistrationSpec* blockRegistrationSpecForId(int blockId) noexcept;
} // namespace net::minecraft::mod::lua
