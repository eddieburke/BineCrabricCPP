#pragma once
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/model/BakedModel.hpp"
namespace net::minecraft {
class ItemStack;
}
namespace net::minecraft::block {
class Block;
}
namespace net::minecraft::client::render {
class Tessellator;
}
namespace net::minecraft::client::render::block {
class BlockRenderManager;
}
namespace net::minecraft::mod::model {
struct ManualBlockVertex {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double u = 0.0;
  double v = 0.0;
};
bool parseModelCallback(lua_State* state, int index, int& ref, std::string& error);
bool emitManualBlockModelQuad(
    const ManualBlockVertex* vertices, int textureId, float red, float green, float blue, float alpha);
bool emitManualItemModelQuad(
    const ManualBlockVertex* vertices, int textureId, float red, float green, float blue, float alpha);
// Emits a baked JSON model (ModelRegistry handle) into the active manual block
// or item draw context, mapping each batch texture into the mod texture atlas.
bool drawBakedModelQuads(int handle);
// Per-coordinate jitter applied to a baked model's quads at draw time (see
// coordinate_bounds/coordinate_color on register_block): scale/offset are
// applied around the model's (0.5, 0.5, 0.5) pivot in model space, color is
// multiplied into each quad's baked color.
struct BakedQuadTransform {
  float scale = 1.0f;
  float offsetX = 0.0f;
  float offsetY = 0.0f;
  float offsetZ = 0.0f;
  float colorR = 1.0f;
  float colorG = 1.0f;
  float colorB = 1.0f;
};
bool drawBakedModelQuads(int handle, const BakedQuadTransform& transform);
bool drawLuaBlockWorld(client::render::block::BlockRenderManager& manager, block::Block& block, int x, int y, int z);
void drawLuaBlockInventory(client::render::block::BlockRenderManager& manager,
                           block::Block& block,
                           int metadata,
                           float brightness);
bool drawLuaItemModel(client::render::Tessellator& tessellator, const ItemStack& stack, float brightness);
// Options for minecraft.model.draw: positions are absolute world coordinates
// (the active render camera is subtracted so mods never handle camera
// offsets); the anchor (x, y, z) lands on model-space (0.5, pivotY, 0.5),
// which is also the yaw/pitch/roll rotation pivot.
struct WorldModelDraw {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  float yaw = 0.0f;
  float pitch = 0.0f;
  float roll = 0.0f;
  float pivotY = 0.0f;
  float scale = 1.0f;
  float brightness = -1.0f;
  float alpha = 1.0f;
  bool blend = true;
  bool cull = false;
  bool depthTest = true;
  bool depthWrite = true;
};
// World-space immediate draw of a baked model; false when the handle is
// unknown, no world draw context is active, or the client renderer is absent.
bool drawBakedModelWorld(int handle, const WorldModelDraw& options);
// World-space draw of an item stack using its *real* model: a custom Lua
// item/block baked model, or the vanilla/mod block-cube renderer (same path
// vanilla uses for dropped block items and inventory icons) for full block
// items. Returns false for plain sprite items (tools, food, ...) so callers
// fall back to their own flat-icon representation.
bool drawItemStackWorld(const ItemStack& stack, const WorldModelDraw& options);
// Model-space bounds (pre-scale, pre-transform) for the same items
// drawItemStackWorld draws. Returns false (bounds left untouched) for plain
// sprite items with no real 3D shape to measure.
bool itemStackBounds(const ItemStack& stack, BakedBounds& outBounds);
} // namespace net::minecraft::mod::model
