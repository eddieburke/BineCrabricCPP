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
 float u = 0.0f;
 float v = 0.0f;
};
struct BakedQuad {
 ModelFace face = ModelFace::Down;
 int cullFace = -1;
 float shade = 1.0f;
 bool coplanarBackFace = false;
 int tintIndex = -1;
 float red = 1.0f;
 float green = 1.0f;
 float blue = 1.0f;
 float alpha = 1.0f;
 BakedVertex vertices[4];
};
struct BakedTextureBatch {
 std::string texturePath;
 int textureId = -1;
 std::vector<BakedQuad> quads;
};
struct BakedBounds {
 float min[3] = {0.0f, 0.0f, 0.0f};
 float max[3] = {0.0f, 0.0f, 0.0f};
 bool empty = true;
};
struct BakedModel {
 std::vector<BakedTextureBatch> batches;
 BakedBounds bounds;
};
void computeBakedBounds(BakedModel& model);
int loadBakedModel(const std::string& modId, const std::string& path, std::string& error);
[[nodiscard]] int bakedModelHandleForKey(const std::string& key) noexcept;
int storeBakedModel(const std::string& key, std::unique_ptr<BakedModel> baked);
[[nodiscard]] const BakedModel* bakedModelForHandle(int handle) noexcept;
[[nodiscard]] int bakedModelTextureId(int handle) noexcept;
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
bool raycastModelInstances(double ox, double oy, double oz, double dx, double dy, double dz, double maxDistance,
                           ModelRaycastHit& hit);
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
 bool blend = false;
 bool cull = true;
 bool depthTest = true;
 bool depthWrite = true;
 runtime::ModDrawLayer layer = runtime::ModDrawLayer::Auto;
 int entityId = 0;
 std::string shaderEntity;
 int blockId = 0;
 int blockMeta = 0;
};
bool drawBakedModelWorld(int handle, const WorldModelDraw& options);
bool drawItemStackWorld(const ItemStack& stack, const WorldModelDraw& options);
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
   t.setCaptureOnly(true);
  t.startQuads();
  t.translate(chunkOffX, chunkOffY, chunkOffZ);
  return t;
 }
};
} // namespace net::minecraft::client::render::chunk
