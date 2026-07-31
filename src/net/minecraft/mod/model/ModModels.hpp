#pragma once
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
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
 // Normalized texture coordinates: 0..1 across the batch's own texture image.
 // JSON models divide their face uvs by the model's "texture_size" (Blockbench
 // semantics, default 16x16) to get here, so the texture's real pixel size
 // never enters the mapping. Every draw path consumes this same convention.
 float u = 0.0f;
 float v = 0.0f;
};
struct BakedQuad {
 ModelFace face = ModelFace::Down;
 int cullFace = -1;
 // Vanilla directional shading factor (1.0 when the element disables shade).
 float shade = 1.0f;
 // Historically flagged on the max-side face of a flat element's opposite-face
 // pair. The baker now drops those twins at bake time; draw paths still skip
 // if set so older/cached meshes cannot Z-fight under cull-off.
 bool coplanarBackFace = false;
 int tintIndex = -1;
 float red = 1.0f;
 float green = 1.0f;
 float blue = 1.0f;
 float alpha = 1.0f;
 BakedVertex vertices[4];
};
// Quads grouped by resolved texture path so draws bind each texture once.
// An empty texturePath marks an untextured batch colored per quad; its
// textureId stays -1 so block draws fall back to the block's own texture.
struct BakedTextureBatch {
 std::string texturePath;
 int textureId = -1;
 std::vector<BakedQuad> quads;
};
// Axis-aligned bounds over all baked vertices, in model space (0..1 units).
struct BakedBounds {
 float min[3] = {0.0f, 0.0f, 0.0f};
 float max[3] = {0.0f, 0.0f, 0.0f};
 bool empty = true;
};
// Immutable once stored. Chunk-mesh workers read baked models off the render
// thread, so nothing here may be written after baking — the GPU buffers a world
// draw needs live in a render-thread-owned cache keyed by handle instead of
// being lazily filled through a const model.
struct BakedModel {
 std::vector<BakedTextureBatch> batches;
 BakedBounds bounds;
};
// Recomputes bounds from the current batches; call after building quads and
// before the model is handed to storeBakedModel (which freezes it).
void computeBakedBounds(BakedModel& model);
// Loads and bakes a JSON model from a mod's assets (resolving parent chains),
// caching by (modId, path). Returns a handle >= 1, or 0 with error set.
int loadBakedModel(const std::string& modId, const std::string& path, std::string& error);
// Cache entry points for external bakers (e.g. voxel sprite extrusion).
[[nodiscard]] int bakedModelHandleForKey(const std::string& key) noexcept;
// Takes ownership and publishes the model as immutable; the handle it returns
// stays valid, and points at the same object, for the rest of the process.
int storeBakedModel(const std::string& key, std::unique_ptr<BakedModel> baked);
// Looks up a previously stored model; nullptr for unknown handles.
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
bool drawLuaBlockWorld(client::render::block::BlockRenderManager& manager, block::Block& block, int x, int y, int z);
void drawLuaBlockInventory(client::render::block::BlockRenderManager& manager,
                           block::Block& block,
                           int metadata,
                           float brightness);
bool drawLuaItemModel(client::render::Tessellator& tessellator, const ItemStack& stack, float brightness);
// Draws a 2.5D extruded sprite at the origin (x=[0,1], y=[0,1], z=[-depth,0])
// using front/back faces and 16 edge slices per axis. Used by both
// HeldItemRenderer (first-person) and drawItemStackWorld (world drops).
void drawExtrudedSprite(client::render::Tessellator& tess,
                        float uMin, float uMax, float vMin, float vMax);
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
 // Opaque props are the common case; translucent opt-in matches vanilla entities.
 bool blend = false;
 bool cull = true;
 bool depthTest = true;
 bool depthWrite = true;
 runtime::ModDrawLayer layer = runtime::ModDrawLayer::Auto;
 int entityId = 0;
 // Optional name for resolveShaderObjectId("entity", …) when entityId is 0.
 std::string shaderEntity;
 // Optional mc_Entity.x (and metadata) for terrain/block layers.
 int blockId = 0;
 int blockMeta = 0;
};
// World-space immediate draw of a baked model; false when the handle is
// unknown, no world draw context is active, or the client renderer is absent.
bool drawBakedModelWorld(int handle, const WorldModelDraw& options);
// World-space draw of an item stack using its *real* model: a custom Lua
// item/block baked model, the vanilla/mod block-cube renderer for full block
// items, or a 2.5D extruded sprite for flat items (tools, food, ...). Returns
// false only when no world draw context is active or the client is absent.
bool drawItemStackWorld(const ItemStack& stack, const WorldModelDraw& options);
// Model-space bounds (pre-scale, pre-transform) for the same items
// drawItemStackWorld draws. Covers custom models, block models, and flat
// sprite items (the latter using the 2.5D extrusion footprint).
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
