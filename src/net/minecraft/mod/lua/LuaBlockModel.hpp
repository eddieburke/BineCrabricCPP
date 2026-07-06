#pragma once
#include "net/minecraft/util/math/Types.hpp"
#include <optional>
#include <string>
#include <vector>
struct lua_State;
namespace net::minecraft::block {
class Block;
}
namespace net::minecraft::client::render::block {
class BlockRenderManager;
}
namespace net::minecraft::mod::lua {
enum class ConnectionRule {
  Same = 0,
  Opaque,
  Glass,
  Fence,
};
struct ModelBox {
  float minX = 0.0f;
  float minY = 0.0f;
  float minZ = 0.0f;
  float maxX = 1.0f;
  float maxY = 1.0f;
  float maxZ = 1.0f;
  bool alwaysDraw = true;
  int connectNorth = 0;
  int connectSouth = 0;
  int connectEast = 0;
  int connectWest = 0;
};
struct LuaBlockModelSpec {
  enum class Type {
    FullCube = 0,
    BoxList,
    ConnectedBars,
  };
  Type type = Type::FullCube;
  std::vector<ModelBox> boxes;
  std::vector<ConnectionRule> connectRules;
  float collisionHeight = 1.0f;
  bool opaque = true;
  bool fullCube = true;
  bool stackOnSame = false;
  bool requiresSolidBelow = true;
  bool coordinateBounds = false;
  bool coordinateColor = false;
  float boundsPadding = 0.0625f;
  float boundsOffset = 0.1f;
  float minScale = 0.9f;
  float maxScale = 1.1f;
};
struct BlockRegistrationSpec {
  int blockId = 0;
  std::string texturePath;
  int terrainTextureId = -1;
  float hardness = 1.0f;
  float resistance = 1.0f;
  float luminance = 0.0f;
  std::string translationKey;
  std::string material = "stone";
  LuaBlockModelSpec model;
};
bool registerBlockSpec(const BlockRegistrationSpec& spec, std::string& error);
[[nodiscard]] const BlockRegistrationSpec* blockRegistrationSpecForId(int blockId) noexcept;
bool drawLuaBlockWorld(client::render::block::BlockRenderManager& manager, block::Block& block, int x, int y, int z);
void drawLuaBlockInventory(client::render::block::BlockRenderManager& manager, block::Block& block, int metadata,
                           float brightness);
} // namespace net::minecraft::mod::lua
