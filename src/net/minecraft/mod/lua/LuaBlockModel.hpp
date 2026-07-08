#pragma once
#include "net/minecraft/util/math/Types.hpp"
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
    Manual,
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
// Registration input lives in LuaBlockRegistry; the model layer only needs to
// name it for the manual-draw and instantiation entry points below.
struct BlockRegistrationSpec;
struct ManualBlockVertex {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double u = 0.0;
  double v = 0.0;
};
// Builds the concrete Block from a validated spec. Called by the block registry
// during the batched BlockRegistration lifecycle phase.
void instantiateLuaModBlock(const BlockRegistrationSpec& spec);
bool invokeManualBlockModelDraw(const BlockRegistrationSpec& spec, bool inventory, int x, int y, int z,
                                float brightness);
bool emitManualBlockModelQuad(const ManualBlockVertex* vertices, int textureId, float red, float green, float blue,
                              float alpha);
bool drawLuaBlockWorld(client::render::block::BlockRenderManager& manager, block::Block& block, int x, int y, int z);
void drawLuaBlockInventory(client::render::block::BlockRenderManager& manager, block::Block& block, int metadata,
                           float brightness);
} // namespace net::minecraft::mod::lua
