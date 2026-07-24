#pragma once
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
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
// Face order matches vanilla Direction ordinals: down, up, north, south, west, east.
enum class ModelFace {
 Down = 0,
 Up,
 North,
 South,
 West,
 East
};
inline constexpr int kModelFaceCount = 6;
struct BakedVertex {
 float x = 0.0f;
 float y = 0.0f;
 float z = 0.0f;
 // Normalized texture coordinates (0..1 across the face's texture).
 float u = 0.0f;
 float v = 0.0f;
};
struct BakedQuad {
 ModelFace face = ModelFace::Down;
 int cullFace = -1;
 // Vanilla directional shading factor (1.0 when the element disables shade).
 float shade = 1.0f;
 int tintIndex = -1;
 float red = 1.0f;
 float green = 1.0f;
 float blue = 1.0f;
 float alpha = 1.0f;
 BakedVertex vertices[4];
};
// Quads grouped by resolved texture path so draws bind each texture once.
// An empty texturePath marks an untextured batch colored per quad.
struct BakedTextureBatch {
 std::string texturePath;
 int textureId = 0;
 std::vector<BakedQuad> quads;
};
// Axis-aligned bounds over all baked vertices, in model space (0..1 units).
struct BakedBounds {
 float min[3] = {0.0f, 0.0f, 0.0f};
 float max[3] = {0.0f, 0.0f, 0.0f};
 bool empty = true;
};
struct BakedModel {
 std::vector<BakedTextureBatch> batches;
 BakedBounds bounds;
 // GPU-uploaded vertex buffers (one per batch), lazily built on first world
 // draw. Only accessed from the render thread; mutable so const lookups can
 // cache them without external maps.
 mutable std::vector<net::minecraft::client::render::TessellatorMesh> gpuMeshes;
 mutable bool gpuMeshesBuilt = false;
};
// Recomputes bounds from the current batches; call after building quads.
void computeBakedBounds(BakedModel& model);
// Loads and bakes a JSON model from a mod's assets (resolving parent chains),
// caching by (modId, path). Returns a handle >= 1, or 0 with error set.
int loadBakedModel(const std::string& modId, const std::string& path, std::string& error);
// Cache entry points for external bakers (e.g. voxel sprite extrusion).
[[nodiscard]] int bakedModelHandleForKey(const std::string& key) noexcept;
int storeBakedModel(const std::string& key, std::unique_ptr<BakedModel> baked);
// Looks up a previously loaded model; nullptr for unknown handles.
[[nodiscard]] const BakedModel* bakedModelForHandle(int handle) noexcept;
// Placement transform for a model instance. Matches minecraft.model.draw: the
// anchor (x, y, z) is where model-space (0.5, pivotY, 0.5) lands, and is also
// the rotation pivot. scale applies uniformly about that anchor.
struct ModelTransform {
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 float yaw = 0.0f;
 float pitch = 0.0f;
 float roll = 0.0f;
 float scale = 1.0f;
 float pivotY = 0.0f;
};
// Registers a placed instance of a baked model and returns its instance id
// (>= 1), or 0 if the handle is unknown. The world-space AABB is derived from
// the model's baked bounds and the transform (scale included), so hitboxes
// track global/per-model scaling automatically.
int placeModelInstance(const std::string& modId, int handle, const ModelTransform& transform,
                       const std::string& tag);
bool updateModelInstance(int instanceId, const ModelTransform& transform);
bool removeModelInstance(int instanceId);
void clearModelInstances(const std::string& modId);
struct ModelRaycastHit {
 int instanceId = 0;
 std::string tag;
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double distance = 0.0;
};
// Tests all placed instances against the ray and reports the nearest hit within
// maxDistance. Returns false if nothing is hit.
bool raycastModelInstances(double ox, double oy, double oz, double dx, double dy, double dz, double maxDistance,
                           ModelRaycastHit& hit);
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
// Per-coordinate jitter applied to a baked model's quads at draw time (see
// coordinate_bounds/coordinate_color on register_block): scale/offset are
// applied around the model's (0.5, 0.5, 0.5) pivot in model space, color is
// multiplied into each quad's baked color.
struct BakedQuadTransform {
 float scale = 1.0f;
 float offsetX = 0.0f;
 float offsetY = 0.0f;
 float offsetZ = 0.0f;
 float yaw = 0.0f;
 float pitch = 0.0f;
 float roll = 0.0f;
 float colorR = 1.0f;
 float colorG = 1.0f;
 float colorB = 1.0f;
};
// Emits a baked JSON model (ModelRegistry handle) into the active manual block
// or item draw context, mapping each batch texture into the mod texture atlas.
bool drawBakedModelQuads(int handle, const BakedQuadTransform& transform = {});
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
namespace net::minecraft::mod::runtime {
void installModelApi(lua_State* state, ModHost::LoadedLuaMod& mod);
} // namespace net::minecraft::mod::runtime
namespace net::minecraft::client::render::chunk {
struct ModChunkMesh {
 int texture = 0;
 TessellatorMesh mesh;
};
struct ModMeshCollector {
 struct Entry {
  int texture;
  Tessellator tess;
 };
 std::deque<Entry> entries;
 double chunkOffX = 0.0;
 double chunkOffY = 0.0;
 double chunkOffZ = 0.0;
 Tessellator& tessFor(int textureId, Tessellator& terrain) {
  if(!net::minecraft::registry::TextureRegistry::isCustomTexture(textureId)) {
   return terrain;
  }
  for(Entry& e : entries) {
   if(e.texture == textureId) {
    return e.tess;
   }
  }
  entries.push_back({textureId, Tessellator{}});
  Tessellator& t = entries.back().tess;
  // Mod chunk meshes are built on worker threads alongside the terrain
  // tessellator; capture mode keeps a busy chunk from flushing through GL
  // off the render thread when its vertex buffer crosses the threshold.
  t.setCaptureOnly(true);
  t.startQuads();
  t.translate(chunkOffX, chunkOffY, chunkOffZ);
  return t;
 }
};
} // namespace net::minecraft::client::render::chunk
