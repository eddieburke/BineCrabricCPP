#include "net/minecraft/mod/model/ModModels.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/TextureResolve.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/model/JsonModelBaker.hpp"
#include "net/minecraft/mod/model/JsonModelParser.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/util/math/CoordinateHash.hpp"
#include "net/minecraft/util/math/Intersect.hpp"
#include "net/minecraft/world/BlockView.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/LightType.hpp"
#endif
namespace net::minecraft::mod::model {
namespace core = net::minecraft::client::render::core;
namespace {
constexpr int kFaceOffsets[kModelFaceCount][3] = {
    {0, -1, 0},
    {0, 1, 0},
    {0, 0, -1},
    {0, 0, 1},
    {-1, 0, 0},
    {1, 0, 0},
};
struct ModelStore {
 std::mutex mutex;
 std::vector<std::unique_ptr<const BakedModel>> models;
 std::unordered_map<std::string, int> handlesByKey;
};
ModelStore& store() {
 static ModelStore instance;
 return instance;
}
} // namespace
void computeBakedBounds(BakedModel& model) {
 BakedBounds bounds;
 for(const BakedTextureBatch& batch : model.batches) {
  for(const BakedQuad& quad : batch.quads) {
   for(const BakedVertex& vertex : quad.vertices) {
    const float point[3] = {vertex.x, vertex.y, vertex.z};
    if(bounds.empty) {
     for(int axis = 0; axis < 3; ++axis) {
      bounds.min[axis] = point[axis];
      bounds.max[axis] = point[axis];
     }
     bounds.empty = false;
    } else {
     for(int axis = 0; axis < 3; ++axis) {
      bounds.min[axis] = std::min(bounds.min[axis], point[axis]);
      bounds.max[axis] = std::max(bounds.max[axis], point[axis]);
     }
    }
   }
  }
 }
 model.bounds = bounds;
}
int bakedModelHandleForKey(const std::string& key) noexcept {
 ModelStore& models = store();
 const std::lock_guard<std::mutex> lock(models.mutex);
 const auto it = models.handlesByKey.find(key);
 return it != models.handlesByKey.end() ? it->second : 0;
}
int storeBakedModel(const std::string& key, std::unique_ptr<BakedModel> baked) {
 ModelStore& models = store();
 const std::lock_guard<std::mutex> lock(models.mutex);
 models.models.push_back(std::move(baked));
 const int handle = static_cast<int>(models.models.size());
 models.handlesByKey.emplace(key, handle);
 return handle;
}
int loadBakedModel(const std::string& modId, const std::string& path, std::string& error) {
 const std::string normalizedPath = detail::normalizeModelPath(path);
 const std::string key = modId + "|" + normalizedPath;
 if(const int cached = bakedModelHandleForKey(key)) {
  return cached;
 }
 detail::JsonModel merged;
 if(!detail::loadModelFile(modId, normalizedPath, merged, error)) {
  return 0;
 }
 // Flatten the parent chain. Blockbench exports sometimes carry dangling or
 // self-referential parents; those are skipped rather than fatal.
 std::set<std::string> visited{normalizedPath};
 std::string basePath = detail::directoryOf(normalizedPath);
 std::string parent = merged.parent;
 while(!parent.empty()) {
  const std::string parentPath = detail::parentModelPath(parent, basePath);
  if(!visited.insert(parentPath).second) {
   break;
  }
  detail::JsonModel parentModel;
  std::string parentError;
  if(!detail::loadModelFile(modId, parentPath, parentModel, parentError)) {
   lua::runtimeLog(
       modId, "warn", "model " + normalizedPath + " skipping parent " + parent + " (" + parentError + ")");
   break;
  }
  detail::mergeParentModel(merged, parentModel);
  basePath = detail::directoryOf(parentPath);
  parent = parentModel.parent;
 }
 auto baked = std::make_unique<BakedModel>();
 if(!detail::bakeJsonModel(merged, detail::directoryOf(normalizedPath), *baked, error)) {
  error = normalizedPath + ": " + error;
  return 0;
 }
 return storeBakedModel(key, std::move(baked));
}
const BakedModel* bakedModelForHandle(int handle) noexcept {
 ModelStore& models = store();
 const std::lock_guard<std::mutex> lock(models.mutex);
 if(handle < 1 || static_cast<std::size_t>(handle) > models.models.size()) {
  return nullptr;
 }
 return models.models[static_cast<std::size_t>(handle) - 1].get();
}
namespace {
struct WorldBox {
 double min[3] = {0.0, 0.0, 0.0};
 double max[3] = {0.0, 0.0, 0.0};
 bool valid = false;
};
struct ModelInstance {
 int id = 0;
 int handle = 0;
 std::string modId;
 std::string tag;
 WorldBox box;
};
struct InstanceStore {
 std::mutex mutex;
 std::vector<ModelInstance> instances;
 int nextId = 1;
};
InstanceStore& instanceStore() {
 static InstanceStore instance;
 return instance;
}
// Applies the draw transform (yaw*pitch*roll, scale, pivot) to a model-space
// point and returns its world position.
void transformPoint(const ModelTransform& t, double px, double py, double pz, double* out) {
 double point[3] = {(px - 0.5) * t.scale, (py - t.pivotY) * t.scale, (pz - 0.5) * t.scale};
 static constexpr double origin[3] = {0.0, 0.0, 0.0};
 if(t.roll != 0.0f) {
  detail::rotatePoint(point, origin, 'z', t.roll);
 }
 if(t.pitch != 0.0f) {
  detail::rotatePoint(point, origin, 'x', t.pitch);
 }
 if(t.yaw != 0.0f) {
  detail::rotatePoint(point, origin, 'y', t.yaw);
 }
 out[0] = t.x + point[0];
 out[1] = t.y + point[1];
 out[2] = t.z + point[2];
}
WorldBox worldBoxFor(int handle, const ModelTransform& transform) {
 WorldBox box;
 const BakedModel* baked = bakedModelForHandle(handle);
 if(baked == nullptr || baked->bounds.empty) {
  return box;
 }
 const BakedBounds& b = baked->bounds;
 for(int corner = 0; corner < 8; ++corner) {
  const double px = (corner & 1) ? b.max[0] : b.min[0];
  const double py = (corner & 2) ? b.max[1] : b.min[1];
  const double pz = (corner & 4) ? b.max[2] : b.min[2];
  double world[3];
  transformPoint(transform, px, py, pz, world);
  if(!box.valid) {
   for(int axis = 0; axis < 3; ++axis) {
    box.min[axis] = world[axis];
    box.max[axis] = world[axis];
   }
   box.valid = true;
  } else {
   for(int axis = 0; axis < 3; ++axis) {
    box.min[axis] = std::min(box.min[axis], world[axis]);
    box.max[axis] = std::max(box.max[axis], world[axis]);
   }
  }
 }
 return box;
}
double boxRayEntry(const WorldBox& box, const double* origin, const double* dir, double maxDistance) {
 return util::math::raySlabIntersect(box.min, box.max, origin, dir, maxDistance, 1.0e-9);
}
} // namespace
int placeModelInstance(const std::string& modId, int handle, const ModelTransform& transform, const std::string& tag) {
 const WorldBox box = worldBoxFor(handle, transform);
 if(!box.valid) {
  return 0;
 }
 InstanceStore& instances = instanceStore();
 const std::lock_guard<std::mutex> lock(instances.mutex);
 ModelInstance instance;
 instance.id = instances.nextId++;
 instance.handle = handle;
 instance.modId = modId;
 instance.tag = tag;
 instance.box = box;
 instances.instances.push_back(std::move(instance));
 return instances.instances.back().id;
}
bool updateModelInstance(int instanceId, const ModelTransform& transform) {
 InstanceStore& instances = instanceStore();
 const std::lock_guard<std::mutex> lock(instances.mutex);
 for(ModelInstance& instance : instances.instances) {
  if(instance.id != instanceId) {
   continue;
  }
  const WorldBox box = worldBoxFor(instance.handle, transform);
  if(!box.valid) {
   return false;
  }
  instance.box = box;
  return true;
 }
 return false;
}
bool removeModelInstance(int instanceId) {
 InstanceStore& instances = instanceStore();
 const std::lock_guard<std::mutex> lock(instances.mutex);
 const auto it = std::find_if(instances.instances.begin(),
                              instances.instances.end(),
                              [instanceId](const ModelInstance& i) { return i.id == instanceId; });
 if(it == instances.instances.end()) {
  return false;
 }
 instances.instances.erase(it);
 return true;
}
void clearModelInstances(const std::string& modId) {
 InstanceStore& instances = instanceStore();
 const std::lock_guard<std::mutex> lock(instances.mutex);
 instances.instances.erase(std::remove_if(instances.instances.begin(),
                                          instances.instances.end(),
                                          [&modId](const ModelInstance& i) { return i.modId == modId; }),
                           instances.instances.end());
}
bool raycastModelInstances(
    double ox, double oy, double oz, double dx, double dy, double dz, double maxDistance, ModelRaycastHit& hit) {
 const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
 if(length < 1.0e-9) {
  return false;
 }
 const double origin[3] = {ox, oy, oz};
 const double dir[3] = {dx / length, dy / length, dz / length};
 InstanceStore& instances = instanceStore();
 const std::lock_guard<std::mutex> lock(instances.mutex);
 double bestDistance = maxDistance;
 const ModelInstance* best = nullptr;
 for(const ModelInstance& instance : instances.instances) {
  const double entry = boxRayEntry(instance.box, origin, dir, maxDistance);
  if(entry < 0.0 || entry >= bestDistance) {
   continue;
  }
  bestDistance = entry;
  best = &instance;
 }
 if(best == nullptr) {
  return false;
 }
 hit.instanceId = best->id;
 hit.tag = best->tag;
 hit.distance = bestDistance;
 hit.x = origin[0] + dir[0] * bestDistance;
 hit.y = origin[1] + dir[1] * bestDistance;
 hit.z = origin[2] + dir[2] * bestDistance;
 return true;
}
using namespace net::minecraft::mod::lua;
using namespace net::minecraft::client::render::item;
namespace {
using net::minecraft::BlockView;
using net::minecraft::block::Block;
using net::minecraft::client::render::Tessellator;
using net::minecraft::client::render::block::BlockRenderManager;
#ifdef MINECRAFT_NATIVE_EXPORTS
using runtime::ModLuaDrawScope;
// Camera-relative placement shared by drawBakedModelWorld and drawItemStackWorld.
// Leaves the caller to apply whatever model-space recentring its geometry needs.
void applyWorldDrawTransform(const WorldModelDraw& options) {
 const client::render::FrameRenderCamera& camera = client::render::RenderCameraState::instance().frame();
 core::modelViewStack().translate(static_cast<float>(options.x - camera.x),
                                  static_cast<float>(options.y - camera.y),
                                  static_cast<float>(options.z - camera.z));
 if(options.yaw != 0.0f) {
  core::modelViewStack().rotate(options.yaw, 0.0f, 1.0f, 0.0f);
 }
 if(options.pitch != 0.0f) {
  core::modelViewStack().rotate(options.pitch, 1.0f, 0.0f, 0.0f);
 }
 if(options.roll != 0.0f) {
  core::modelViewStack().rotate(options.roll, 0.0f, 0.0f, 1.0f);
 }
 if(options.scale != 1.0f) {
  core::modelViewStack().scale(options.scale, options.scale, options.scale);
 }
}
// A model is placed by its anchor, and a tall one (a tripod's camera head, a
// sign on a wall) routinely has that anchor land inside terrain: a low ceiling,
// the block it is mounted against. Light inside an opaque block is 0, which
// would render the whole model black while everything around it is lit, so
// borrow the brightest exposed neighbour — the same trick World::getLightLevel
// plays for slabs and stairs.
float worldBrightness(const WorldModelDraw& options) {
 if(options.brightness >= 0.0f) {
  return options.brightness;
 }
 if(client::Minecraft::INSTANCE == nullptr || client::Minecraft::INSTANCE->world == nullptr) {
  return 1.0f;
 }
 World& world = *client::Minecraft::INSTANCE->world;
 const int x = static_cast<int>(std::floor(options.x));
 const int y = static_cast<int>(std::floor(options.y));
 const int z = static_cast<int>(std::floor(options.z));
 float brightness = world.getLightBrightness(x, y, z);
 const int blockId = world.getBlockId(x, y, z);
 if(blockId <= 0 || blockId >= block::Block::BLOCK_COUNT ||
    !block::Block::BLOCKS_OPAQUE[static_cast<std::size_t>(blockId)]) {
  return brightness;
 }
 for(const int (&offset)[3] : kFaceOffsets) {
  brightness = std::max(brightness, world.getLightBrightness(x + offset[0], y + offset[1], z + offset[2]));
 }
 return brightness;
}
#endif
// Where a block's baked model is being drawn. Passed explicitly down the draw
// path; it used to be a thread-local that every emit helper reached for, which
// meant a missing scope guard produced no geometry and no diagnostic.
struct BlockModelDraw {
 BlockRenderManager* manager = nullptr;
 Block* block = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 bool inventory = false;
 float brightness = 1.0f;
};
// ---------------------------------------------------------------------------
// Baked-model quad emission.
//
// One loop turns a BakedModel into quads for every destination — world blocks,
// inventory icons, item models, minecraft.model.draw. A caller supplies where
// each texture batch goes and what to do with a finished quad; the cullface
// test, the per-quad transform, the shade/tint math, the face normal and the uv
// mapping live here and nowhere else.
// A baked vertex after the per-quad transform, in double precision so a world
// draw can carry the block's grid offset without losing model detail.
struct TransformedVertex {
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double u = 0.0;
 double v = 0.0;
};
struct EmittedQuad {
 const TransformedVertex* vertices = nullptr;
 bool coplanarBackFace = false;
 float nx = 0.0f;
 float ny = 1.0f;
 float nz = 0.0f;
 float red = 1.0f;
 float green = 1.0f;
 float blue = 1.0f;
 float alpha = 1.0f;
};
// Face normal from the first three vertices, in the winding the vanilla face
// renderers hand to Tessellator::normal.
void assignQuadNormal(EmittedQuad& quad) {
 client::render::block::quadNormal(quad.vertices[0].x,
                                   quad.vertices[0].y,
                                   quad.vertices[0].z,
                                   quad.vertices[1].x,
                                   quad.vertices[1].y,
                                   quad.vertices[1].z,
                                   quad.vertices[2].x,
                                   quad.vertices[2].y,
                                   quad.vertices[2].z,
                                   quad.nx,
                                   quad.ny,
                                   quad.nz);
}
enum class QuadLightMode {
 // light 0..1 → identical block+sky in vaUV2 (legacy immediate approx).
 Absolute,
 // Match EntityRenderDispatcher: fullbright lightmap; caller puts world
 // brightness in uConstColor / vertex colour.
 Entity,
 // Keep whatever Tessellator::blockData / light already set (chunk mod meshes).
 Preserve,
};
// The one place a baked vertex reaches a Tessellator. Same shape as the vanilla
// face renderers: normal, colour, then four uv-mapped vertices.
void writeQuad(Tessellator& t,
               const EmittedQuad& quad,
               double baseX,
               double baseY,
               double baseZ,
               float light,
               float alphaScale,
               QuadLightMode lightMode = QuadLightMode::Absolute) {
 if(lightMode == QuadLightMode::Absolute) {
  const int level = std::clamp(static_cast<int>(std::lround(light * 15.0f)), 0, 15);
  t.light(level, level);
 } else if(lightMode == QuadLightMode::Entity) {
  t.light(15, 15);
 }
 // Preserve: leave blockLight_/skyLight_ from activeTess::blockData alone.
 t.color(quad.red, quad.green, quad.blue, quad.alpha * alphaScale);
 for(int i = 0; i < 4; ++i) {
  // uv is already normalized 0..1 across the batch's own image, which is what
  // the bound texture is: mod content never lives in the vanilla atlas, so
  // there is no tile to fold these into.
  client::render::block::emitBlockVertex(t,
                                         quad.nx,
                                         quad.ny,
                                         quad.nz,
                                         baseX + quad.vertices[i].x,
                                         baseY + quad.vertices[i].y,
                                         baseZ + quad.vertices[i].z,
                                         quad.vertices[i].u,
                                         quad.vertices[i].v);
 }
}
// beginBatch(batch) -> bool  : false skips the batch (texture unresolvable).
// takeQuad(batch, quad)      : consume one transformed, culled quad.
template <typename BeginBatch, typename TakeQuad>
bool forEachBakedQuad(const BakedModel& baked,
                      const BakedQuadTransform& transform,
                      const BlockModelDraw* cullDraw,
                      const BlockView* cullView,
                      BeginBatch beginBatch,
                      TakeQuad takeQuad) {
 // The overwhelmingly common case: a block or icon drawn at its baked pose.
 // Nothing then needs the per-vertex rotate/scale below, and cullface (which
 // only means anything against the block grid) stays meaningful.
 const bool placedAsBaked = transform.scale == 1.0f && transform.offsetX == 0.0f && transform.offsetY == 0.0f &&
                            transform.offsetZ == 0.0f && transform.yaw == 0.0f && transform.pitch == 0.0f &&
                            transform.roll == 0.0f;
 const bool cullFaces = placedAsBaked && cullDraw != nullptr && cullView != nullptr;
 static constexpr double zeroOrigin[3] = {0.0, 0.0, 0.0};
 bool emitted = false;
 for(const BakedTextureBatch& batch : baked.batches) {
  if(!beginBatch(batch)) {
   continue;
  }
  for(const BakedQuad& quad : batch.quads) {
   if(cullFaces && quad.cullFace >= 0) {
    const int* offset = kFaceOffsets[quad.cullFace];
    if(!cullDraw->block->isSideVisible(
           cullView, cullDraw->x + offset[0], cullDraw->y + offset[1], cullDraw->z + offset[2], quad.cullFace)) {
     continue;
    }
   }
   TransformedVertex vertices[4];
   for(int i = 0; i < 4; ++i) {
    if(placedAsBaked) {
     vertices[i].x = quad.vertices[i].x;
     vertices[i].y = quad.vertices[i].y;
     vertices[i].z = quad.vertices[i].z;
    } else {
     // Rotation and scale act around the model's centre, so shift there first.
     double point[3] = {quad.vertices[i].x - 0.5, quad.vertices[i].y - 0.5, quad.vertices[i].z - 0.5};
     if(transform.pitch != 0.0f) {
      detail::rotatePoint(point, zeroOrigin, 'x', transform.pitch);
     }
     if(transform.yaw != 0.0f) {
      detail::rotatePoint(point, zeroOrigin, 'y', transform.yaw);
     }
     if(transform.roll != 0.0f) {
      detail::rotatePoint(point, zeroOrigin, 'z', transform.roll);
     }
     vertices[i].x = point[0] * transform.scale + 0.5 + transform.offsetX;
     vertices[i].y = point[1] * transform.scale + 0.5 + transform.offsetY;
     vertices[i].z = point[2] * transform.scale + 0.5 + transform.offsetZ;
    }
    vertices[i].u = quad.vertices[i].u;
    vertices[i].v = quad.vertices[i].v;
   }
   EmittedQuad out;
   out.vertices = vertices;
   out.coplanarBackFace = quad.coplanarBackFace;
   out.red = quad.red * quad.shade * transform.colorR;
   out.green = quad.green * quad.shade * transform.colorG;
   out.blue = quad.blue * quad.shade * transform.colorB;
   out.alpha = quad.alpha;
   assignQuadNormal(out);
   takeQuad(batch, out);
   emitted = true;
  }
 }
 return emitted;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
// Render-thread-only scratch tessellator for the world draw paths. Shared
// rather than local because constructing a Tessellator reserves a
// multi-megabyte vertex buffer; every user brackets its own startQuads/draw.
Tessellator& worldDrawTessellator() {
 static Tessellator tess;
 return tess;
}
#endif
// Resolves a block quad's final texture and tessellator and writes it.
bool writeBlockQuad(const BlockModelDraw& draw, const EmittedQuad& quad, int textureId) {
 if(draw.manager == nullptr || draw.block == nullptr) {
  return false;
 }
 // Same rule as minecraft.model.draw: never emit both faces of a thin plane.
 if(quad.coplanarBackFace) {
  return true;
 }
 if(textureId < 0) {
  textureId = draw.block->textureId;
 }
 BlockRenderManager& manager = *draw.manager;
 if(!draw.inventory && manager.ctx.textureOverride >= 0) {
  textureId = manager.ctx.textureOverride;
 }
 manager.ctx.bindTextureFor(textureId);
 Tessellator& t = draw.inventory ? *manager.ctx.tess : manager.ctx.activeTess(textureId);
 // Capture mode owns the batch lifecycle (the chunk builder started it and will
 // draw it); an immediate draw has to bracket every quad itself.
 const bool capturing = !draw.inventory && manager.ctx.modMeshes != nullptr;
 // World quads sit at the block's grid position; inventory quads stay in 0..1.
 const double baseX = draw.inventory ? 0.0 : static_cast<double>(draw.x);
 const double baseY = draw.inventory ? 0.0 : static_cast<double>(draw.y);
 const double baseZ = draw.inventory ? 0.0 : static_cast<double>(draw.z);
 if(!capturing) {
  t.startQuads();
 }
 // World/chunk path: activeTess already wrote face light + mc_Entity. Inventory
 // icons have no blockData — pack luminance into the lightmap from brightness.
 const QuadLightMode lightMode = draw.inventory ? QuadLightMode::Absolute : QuadLightMode::Preserve;
 writeQuad(t, quad, baseX, baseY, baseZ, draw.brightness, 1.0f, lightMode);
 if(!capturing) {
  t.draw();
 }
 return true;
}
bool drawBakedBlockModel(const BlockModelDraw& draw, const BakedModel& baked, const BakedQuadTransform& transform) {
 // Cullface only means anything against the block grid, so an inventory icon
 // keeps every quad.
 const bool grid = !draw.inventory && draw.block != nullptr && draw.manager != nullptr;
 return forEachBakedQuad(baked, transform, grid ? &draw : nullptr, grid ? draw.manager->ctx.blockView : nullptr, [](const BakedTextureBatch&) { return true; }, [&](const BakedTextureBatch& batch, const EmittedQuad& quad) { writeBlockQuad(draw, quad, batch.textureId); });
}
bool drawBakedItemModel(Tessellator& tess, float brightness, const BakedModel& baked) {
 return forEachBakedQuad(baked, BakedQuadTransform{}, nullptr, nullptr, [](const BakedTextureBatch&) { return true; }, [&](const BakedTextureBatch& batch, const EmittedQuad& quad) {
                          if(quad.coplanarBackFace) {
                           return;
                          }
                          if(client::Minecraft::INSTANCE != nullptr) {
                           const client::render::ResolvedTexture resolved = client::render::resolveBlockTexture(
                               batch.textureId,
                               client::Minecraft::INSTANCE->textureManager,
                               client::render::AtlasDomain::Terrain);
                           if(resolved.isModTexture) {
                            client::Minecraft::INSTANCE->textureManager.bindTexture(resolved.glId);
                           }
                          }
                          tess.startQuads();
                          writeQuad(tess, quad, 0.0, 0.0, 0.0, brightness, 1.0f);
                          tess.draw(); });
}
// Baked-model equivalent of LuaModBlock::getRenderBounds/getColorMultiplier:
// register_block's coordinate_bounds/coordinate_color have to be applied here
// too, since a block with a model never takes the vanilla cube path.
BakedQuadTransform coordinateQuadTransform(const BlockRegistrationSpec& spec, int x, int y, int z) {
 BakedQuadTransform transform;
 if(spec.coordinateBounds) {
  const lua::CoordinateVariedTransform varied = lua::coordinateVariedTransform(spec, x, y, z);
  transform.scale = varied.scale;
  transform.offsetX = varied.offsetX;
  transform.offsetY = varied.offsetY;
  transform.offsetZ = varied.offsetZ;
 }
 if(spec.coordinateColor) {
  const int color = net::minecraft::util::math::coordinateColor(x, y, z);
  transform.colorR = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
  transform.colorG = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
  transform.colorB = static_cast<float>(color & 0xFF) / 255.0f;
 }
 return transform;
}
} // namespace
bool drawLuaBlockWorld(BlockRenderManager& manager, Block& block, int x, int y, int z) {
 const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block.id);
 if(spec == nullptr || spec->bakedModel == 0) {
  return false;
 }
 const BakedModel* baked = bakedModelForHandle(spec->bakedModel);
 if(baked == nullptr) {
  return false;
 }
 const BlockModelDraw draw{&manager, &block, x, y, z, false, 1.0f};
 core::setAlphaTestRef(0.1f);
 const BakedQuadTransform transform =
     spec->coordinateBounds || spec->coordinateColor ? coordinateQuadTransform(*spec, x, y, z) : BakedQuadTransform{};
 return drawBakedBlockModel(draw, *baked, transform);
}
void drawLuaBlockInventory(BlockRenderManager& manager, Block& block, int /*metadata*/, float brightness) {
 const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block.id);
 if(spec == nullptr || spec->bakedModel == 0) {
  return;
 }
 const BakedModel* baked = bakedModelForHandle(spec->bakedModel);
 if(baked == nullptr) {
  return;
 }
 core::depthTestWrite(true);
 core::setLightingEnabled(false);
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 const core::ScopedModelView matrix;
 core::modelViewStack().translate(-0.5f, -0.5f, -0.5f);
 const BlockModelDraw draw{&manager, &block, 0, 0, 0, true, brightness};
 drawBakedBlockModel(draw, *baked, BakedQuadTransform{});
}
bool drawLuaItemModel(Tessellator& tessellator, const ItemStack& stack, float brightness) {
 const ItemRegistrationSpec* spec = itemRegistrationSpecForId(stack.itemId);
 if(spec == nullptr || spec->bakedModel == 0) {
  return false;
 }
 const BakedModel* baked = bakedModelForHandle(spec->bakedModel);
 return baked != nullptr && drawBakedItemModel(tessellator, brightness, *baked);
}
#ifdef MINECRAFT_NATIVE_EXPORTS
void sampleWorldBlockSkyLight(const WorldModelDraw& options, int& blockLight, int& skyLight) {
 blockLight = 15;
 skyLight = 15;
 if(client::Minecraft::INSTANCE == nullptr || client::Minecraft::INSTANCE->world == nullptr) {
  return;
 }
 World& world = *client::Minecraft::INSTANCE->world;
 const int x = static_cast<int>(std::floor(options.x));
 const int y = static_cast<int>(std::floor(options.y));
 const int z = static_cast<int>(std::floor(options.z));
 blockLight = world.getBrightness(LightType::Block, x, y, z);
 skyLight = world.getBrightness(LightType::Sky, x, y, z);
 const int blockId = world.getBlockId(x, y, z);
 if(blockId <= 0 || blockId >= block::Block::BLOCK_COUNT ||
    !block::Block::BLOCKS_OPAQUE[static_cast<std::size_t>(blockId)]) {
  return;
 }
 for(const int (&offset)[3] : kFaceOffsets) {
  blockLight =
      std::max(blockLight, world.getBrightness(LightType::Block, x + offset[0], y + offset[1], z + offset[2]));
  skyLight = std::max(skyLight, world.getBrightness(LightType::Sky, x + offset[0], y + offset[1], z + offset[2]));
 }
}
int resolveDrawEntityId(const WorldModelDraw& options) {
 if(!options.shaderEntity.empty()) {
  return client::render::resolveShaderObjectId("entity", options.shaderEntity, options.entityId);
 }
 return options.entityId;
}
bool drawBakedModelWorld(int handle, const WorldModelDraw& options) {
 const BakedModel* baked = bakedModelForHandle(handle);
 if(baked == nullptr || !runtime::ModWorldDrawContext::active() || client::Minecraft::INSTANCE == nullptr) {
  return false;
 }
 const float brightness = worldBrightness(options);
 const bool textured = !baked->batches.empty() && !baked->batches.front().texturePath.empty();
 if(!textured) {
  // Empty / failed bake: refuse gbuffers_basic + white diffuse (classic grey pillar).
  return false;
 }
 const ModLuaDrawScope modCaps(true, options.blend, options.cull, options.depthTest, options.depthWrite,
                               options.layer);
 const client::render::core::EntityIdScope entityIdScope(resolveDrawEntityId(options));
 const bool entityLighting = modCaps.usesEntityLighting();
 if(entityLighting) {
  core::setConstColor(brightness, brightness, brightness, 1.0f);
 }
 if(modCaps.usesTerrainProgram()) {
  core::setPendingTerrainDraw(0.0f, 0.0f, 0.0f);
 }
 const core::ScopedModelView matrix;
 applyWorldDrawTransform(options);
 core::modelViewStack().translate(-0.5f, -options.pivotY, -0.5f);
 client::texture::TextureManager& textures = client::Minecraft::INSTANCE->textureManager;
 Tessellator& tess = worldDrawTessellator();
 int blockLight = 15;
 int skyLight = 15;
 const bool wantBlockData = modCaps.usesTerrainProgram() || options.blockId != 0;
 if(wantBlockData) {
  sampleWorldBlockSkyLight(options, blockLight, skyLight);
 }
 const int shaderBlockId =
     options.blockId != 0 ? client::render::resolveShaderBlockId(options.blockId) : 0;
 const QuadLightMode lightMode =
     entityLighting ? QuadLightMode::Entity
                    : (wantBlockData ? QuadLightMode::Preserve : QuadLightMode::Absolute);
 bool open = false;
 bool drew = false;
 forEachBakedQuad(
     *baked,
     BakedQuadTransform{},
     nullptr,
     nullptr,
     [&](const BakedTextureBatch& batch) {
      if(batch.texturePath.empty()) {
       return false;
      }
      const int glId = textures.getTextureId(batch.texturePath);
      if(glId < 0) {
       return false;
      }
      if(open) {
       tess.draw();
      }
      textures.bindTexture(glId);
      tess.startQuads();
      if(wantBlockData) {
       tess.blockData(options.x, options.y, options.z, 0, blockLight, skyLight, shaderBlockId, 0,
                      options.blockMeta);
      }
      open = true;
      return true;
     },
     [&](const BakedTextureBatch& /*batch*/, const EmittedQuad& quad) {
      // Thin-plane face pairs are coplanar; keep the front of the pair only.
      if(quad.coplanarBackFace) {
       return;
      }
      writeQuad(tess, quad, 0.0, 0.0, 0.0, brightness, options.alpha, lightMode);
      drew = true;
     });
 if(open) {
  tess.draw();
 }
 if(modCaps.usesTerrainProgram()) {
  core::clearPendingTerrainDraw();
 }
 return drew;
}
// Draws a 2.5D extruded sprite for a flat item (front face, back face, and 16
// edge slices along each axis). Shared between HeldItemRenderer and world-item
// draws so the voxel extrusion is authored once. The geometry is emitted at the
// origin spanning x=[0,1], y=[0,1], z=[-depth,0]; callers position it.
void drawExtrudedSprite(Tessellator& tess, float uMin, float uMax, float vMin, float vMax) {
 constexpr float itemWidth = 1.0f;
 constexpr float depth = 0.0625f;
 tess.startQuads();
 // Front face (z = 0)
 tess.normal(0.0f, 0.0f, 1.0f);
 tess.vertex(0.0, 0.0, 0.0, uMax, vMax);
 tess.vertex(itemWidth, 0.0, 0.0, uMin, vMax);
 tess.vertex(itemWidth, 1.0, 0.0, uMin, vMin);
 tess.vertex(0.0, 1.0, 0.0, uMax, vMin);
 // Back face (z = -depth)
 tess.normal(0.0f, 0.0f, -1.0f);
 tess.vertex(0.0, 1.0, 0.0 - depth, uMax, vMin);
 tess.vertex(itemWidth, 1.0, 0.0 - depth, uMin, vMin);
 tess.vertex(itemWidth, 0.0, 0.0 - depth, uMin, vMax);
 tess.vertex(0.0, 0.0, 0.0 - depth, uMax, vMax);
 // Left edge (x slices, normal -X)
 tess.normal(-1.0f, 0.0f, 0.0f);
 for(int slice = 0; slice < 16; ++slice) {
  const float t = static_cast<float>(slice) / 16.0f;
  const float u = uMax + (uMin - uMax) * t - 0.001953125f;
  const float x = itemWidth * t;
  tess.vertex(x, 0.0, 0.0 - depth, u, vMax);
  tess.vertex(x, 0.0, 0.0, u, vMax);
  tess.vertex(x, 1.0, 0.0, u, vMin);
  tess.vertex(x, 1.0, 0.0 - depth, u, vMin);
 }
 // Right edge (x slices, normal +X)
 tess.normal(1.0f, 0.0f, 0.0f);
 for(int slice = 0; slice < 16; ++slice) {
  const float t = static_cast<float>(slice) / 16.0f;
  const float u = uMax + (uMin - uMax) * t - 0.001953125f;
  const float x = itemWidth * t + 0.0625f;
  tess.vertex(x, 1.0, 0.0 - depth, u, vMin);
  tess.vertex(x, 1.0, 0.0, u, vMin);
  tess.vertex(x, 0.0, 0.0, u, vMax);
  tess.vertex(x, 0.0, 0.0 - depth, u, vMax);
 }
 // Top edge (y slices, normal +Y)
 tess.normal(0.0f, 1.0f, 0.0f);
 for(int slice = 0; slice < 16; ++slice) {
  const float t = static_cast<float>(slice) / 16.0f;
  const float v = vMax + (vMin - vMax) * t - 0.001953125f;
  const float y = itemWidth * t + 0.0625f;
  tess.vertex(0.0, y, 0.0, uMax, v);
  tess.vertex(itemWidth, y, 0.0, uMin, v);
  tess.vertex(itemWidth, y, 0.0 - depth, uMin, v);
  tess.vertex(0.0, y, 0.0 - depth, uMax, v);
 }
 // Bottom edge (y slices, normal -Y)
 tess.normal(0.0f, -1.0f, 0.0f);
 for(int slice = 0; slice < 16; ++slice) {
  const float t = static_cast<float>(slice) / 16.0f;
  const float v = vMax + (vMin - vMax) * t - 0.001953125f;
  const float y = itemWidth * t;
  tess.vertex(itemWidth, y, 0.0, uMin, v);
  tess.vertex(0.0, y, 0.0, uMax, v);
  tess.vertex(0.0, y, 0.0 - depth, uMax, v);
  tess.vertex(itemWidth, y, 0.0 - depth, uMin, v);
 }
 tess.draw();
}
bool drawItemStackWorld(const ItemStack& stack, const WorldModelDraw& options) {
 if(!runtime::ModWorldDrawContext::active() || client::Minecraft::INSTANCE == nullptr) {
  return false;
 }
 const bool custom = ItemModelRenderer::hasCustomModel(stack);
 const bool blockModel = !custom && ItemModelRenderer::rendersAsBlockModel(stack);
 Block* block = blockModel ? ItemModelRenderer::blockOf(stack) : nullptr;
 if(blockModel && block == nullptr) {
  return false;
 }
 const float brightness = worldBrightness(options);
 const ModLuaDrawScope modCaps(true, options.blend, options.cull, options.depthTest, options.depthWrite,
                               options.layer);
 const client::render::core::EntityIdScope entityIdScope(resolveDrawEntityId(options));
 if(modCaps.usesEntityLighting()) {
  core::setConstColor(brightness, brightness, brightness, 1.0f);
 }
 const core::ScopedModelView matrix;
 applyWorldDrawTransform(options);
 client::texture::TextureManager& textures = client::Minecraft::INSTANCE->textureManager;
 if(custom) {
  // Custom item models are baked in 0..1 model space; recentre onto the pivot.
  core::modelViewStack().translate(-0.5f, -options.pivotY, -0.5f);
  textures.bindTexture(
      client::render::resolveBlockTexture(stack.getTextureId(), textures, ItemModelRenderer::atlasDomain(stack)).glId);
  // Entity lighting: brightness already in uConstColor; lightmap fullbright.
  return drawLuaItemModel(worldDrawTessellator(), stack, modCaps.usesEntityLighting() ? 1.0f : brightness);
 }
 if(blockModel) {
  // The inventory block renderers emit geometry already centred on the origin,
  // so only the pivot's deviation from centre needs compensating.
  if(options.pivotY != 0.5f) {
   core::modelViewStack().translate(0.0f, 0.5f - options.pivotY, 0.0f);
  }
  textures.bindTexture(client::render::resolveBlockTexture(block->textureId, textures,
                                                            client::render::AtlasDomain::Terrain)
                           .glId);
  static BlockRenderManager itemDropBlockManager;
  auto* previousTextureManager = itemDropBlockManager.ctx.textureManager;
  const bool previousUseAo = itemDropBlockManager.ctx.faceState.useAo;
  itemDropBlockManager.ctx.textureManager = &textures;
  itemDropBlockManager.ctx.faceState.useAo = false;
  itemDropBlockManager.render(*block, stack.getDamage(), brightness);
  itemDropBlockManager.ctx.textureManager = previousTextureManager;
  itemDropBlockManager.ctx.faceState.useAo = previousUseAo;
  return true;
 }
 // Flat sprite items (tools, food, ...) get a 2.5D extruded icon. The
 // geometry spans x=[0,1], y=[0,1], z=[-depth,0]; recentre onto the pivot.
 const auto uv = ItemModelRenderer::spriteUv(stack);
 core::modelViewStack().translate(-0.5f, -options.pivotY, 0.0625f * 0.5f);
 textures.bindTexture(
     client::render::resolveBlockTexture(stack.getTextureId(), textures, ItemModelRenderer::atlasDomain(stack)).glId);
 {
  Tessellator& tess = worldDrawTessellator();
  const int level = std::clamp(static_cast<int>(std::lround(brightness * 15.0f)), 0, 15);
  tess.light(level, level);
  core::setConstColor(1.0f, 1.0f, 1.0f, options.alpha);
  drawExtrudedSprite(tess,
                     static_cast<float>(uv.uMin), static_cast<float>(uv.uMax),
                     static_cast<float>(uv.vMin), static_cast<float>(uv.vMax));
 }
 return true;
}
#else
bool drawBakedModelWorld(int /*handle*/, const WorldModelDraw& /*options*/) {
 return false;
}
bool drawItemStackWorld(const ItemStack& /*stack*/, const WorldModelDraw& /*options*/) {
 return false;
}
#endif
bool itemStackBounds(const ItemStack& stack, BakedBounds& outBounds) {
 if(ItemModelRenderer::hasCustomModel(stack)) {
  const ItemRegistrationSpec* spec = itemRegistrationSpecForId(stack.itemId);
  if(spec == nullptr || spec->bakedModel == 0) {
   return false;
  }
  const BakedModel* baked = bakedModelForHandle(spec->bakedModel);
  if(baked == nullptr || baked->bounds.empty) {
   return false;
  }
  outBounds = baked->bounds;
  return true;
 }
 if(ItemModelRenderer::rendersAsBlockModel(stack)) {
  Block* block = ItemModelRenderer::blockOf(stack);
  if(block == nullptr) {
   return false;
  }
  const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block->id);
  if(spec != nullptr && spec->bakedModel != 0) {
   const BakedModel* baked = bakedModelForHandle(spec->bakedModel);
   if(baked != nullptr && !baked->bounds.empty) {
    outBounds = baked->bounds;
    return true;
   }
  }
  // Vanilla (and shape-less custom) blocks: approximate with the full unit
  // cube. Non-cube shapes (stairs, fences, ...) are subsets of this box, so
  // it is a safe (if slightly loose) tumble hitbox rather than an exact one.
  outBounds.min[0] = outBounds.min[1] = outBounds.min[2] = 0.0f;
  outBounds.max[0] = outBounds.max[1] = outBounds.max[2] = 1.0f;
  outBounds.empty = false;
  return true;
 }
 // Flat sprite items: match the extruded 2.5D icon geometry (x=[0,1], y=[0,1],
 // z=[-depth,0]). Centre is (0.5, 0.5, -depth/2) which aligns with the
 // pivot used by drawItemStackWorld.
 constexpr float depth = 0.0625f;
 outBounds.min[0] = 0.0f;
 outBounds.min[1] = 0.0f;
 outBounds.min[2] = -depth;
 outBounds.max[0] = 1.0f;
 outBounds.max[1] = 1.0f;
 outBounds.max[2] = 0.0f;
 outBounds.empty = false;
 return true;
}
} // namespace net::minecraft::mod::model
namespace net::minecraft::mod::runtime {
namespace core = net::minecraft::client::render::core;
using namespace net::minecraft::mod::lua;
namespace {
int luaModelLoad(lua_State* state) {
 LuaApi& api = luaApi();
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 const std::string path = luaString(state, 1, "");
 if(mod == nullptr || path.empty()) {
  api.pushnil(state);
  api.pushstring(state, "model.load expects an asset path");
  return 2;
 }
 std::string error;
 const int handle = model::loadBakedModel(mod->modId, path, error);
 if(handle == 0) {
  api.pushnil(state);
  api.pushstring(state, error.c_str());
  return 2;
 }
 api.pushinteger(state, handle);
 return 1;
}
// Reads a placement transform from the options table at optsIndex.
model::ModelTransform readTransform(lua_State* state, int optsIndex) {
 model::ModelTransform t;
 if(optsIndex != 0) {
  t.x = luaDoubleField(state, optsIndex, "x", 0.0);
  t.y = luaDoubleField(state, optsIndex, "y", 0.0);
  t.z = luaDoubleField(state, optsIndex, "z", 0.0);
  t.yaw = luaFloatField(state, optsIndex, "yaw", 0.0f);
  t.pitch = luaFloatField(state, optsIndex, "pitch", 0.0f);
  t.roll = luaFloatField(state, optsIndex, "roll", 0.0f);
  t.scale = luaFloatField(state, optsIndex, "scale", 1.0f);
  t.pivotY = luaFloatField(state, optsIndex, "pivot_y", 0.0f);
 }
 return t;
}
// model.place(handle, opts) -> instance id. Registers a hitbox the engine's
// raycast honors; the box tracks the transform's scale automatically.
int luaModelPlace(lua_State* state) {
 LuaApi& api = luaApi();
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 const int handle = luaIntArg(state, 1, 0);
 if(mod == nullptr || handle <= 0) {
  api.pushnil(state);
  api.pushstring(state, "model.place expects (handle, opts)");
  return 2;
 }
 const int optsIndex = api.type(state, 2) == kLuaTTable ? 2 : 0;
 const model::ModelTransform transform = readTransform(state, optsIndex);
 const std::string tag = optsIndex != 0 ? luaStringField(state, optsIndex, "tag", "") : std::string();
 const int instanceId = model::placeModelInstance(mod->modId, handle, transform, tag);
 if(instanceId == 0) {
  api.pushnil(state);
  api.pushstring(state, "model.place failed: unknown or empty model");
  return 2;
 }
 api.pushinteger(state, instanceId);
 return 1;
}
int luaModelUpdate(lua_State* state) {
 LuaApi& api = luaApi();
 const int instanceId = luaIntArg(state, 1, 0);
 const int optsIndex = api.type(state, 2) == kLuaTTable ? 2 : 0;
 const model::ModelTransform transform = readTransform(state, optsIndex);
 api.pushboolean(state, model::updateModelInstance(instanceId, transform) ? 1 : 0);
 return 1;
}
int luaModelRemove(lua_State* state) {
 LuaApi& api = luaApi();
 api.pushboolean(state, model::removeModelInstance(luaIntArg(state, 1, 0)) ? 1 : 0);
 return 1;
}
int luaModelClear(lua_State* state) {
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod != nullptr) {
  model::clearModelInstances(mod->modId);
 }
 return 0;
}
int luaModelBounds(lua_State* state) {
 LuaApi& api = luaApi();
 const model::BakedModel* baked = model::bakedModelForHandle(luaIntArg(state, 1, 0));
 if(baked == nullptr || baked->bounds.empty) {
  api.pushnil(state);
  return 1;
 }
 const model::BakedBounds& b = baked->bounds;
 api.createtable(state, 0, 6);
 setFields(state,
           "min_x",
           static_cast<double>(b.min[0]),
           "min_y",
           static_cast<double>(b.min[1]),
           "min_z",
           static_cast<double>(b.min[2]),
           "max_x",
           static_cast<double>(b.max[0]),
           "max_y",
           static_cast<double>(b.max[1]),
           "max_z",
           static_cast<double>(b.max[2]));
 return 1;
}
// Shared option parsing for minecraft.model.draw and minecraft.model.draw_item.
model::WorldModelDraw readWorldModelDraw(lua_State* state, int optsIndex) {
 model::WorldModelDraw options;
 if(optsIndex != 0) {
  options.x = luaDoubleField(state, optsIndex, "x", 0.0);
  options.y = luaDoubleField(state, optsIndex, "y", 0.0);
  options.z = luaDoubleField(state, optsIndex, "z", 0.0);
  options.yaw = luaFloatField(state, optsIndex, "yaw", 0.0f);
  options.pitch = luaFloatField(state, optsIndex, "pitch", 0.0f);
  options.roll = luaFloatField(state, optsIndex, "roll", 0.0f);
  options.pivotY = luaFloatField(state, optsIndex, "pivot_y", 0.0f);
  options.scale = luaFloatField(state, optsIndex, "scale", 1.0f);
  options.brightness = luaFloatField(state, optsIndex, "brightness", -1.0f);
  if(options.brightness >= 0.0f) {
   options.brightness = std::clamp(options.brightness, 0.0f, 1.0f);
  }
  options.alpha = std::clamp(luaFloatField(state, optsIndex, "a", 1.0f), 0.0f, 1.0f);
  // Opaque-first default: translucent is opt-in (entity cutout / terrain cutout).
  options.blend = luaBoolField(state, optsIndex, "blend", false);
  options.cull = luaBoolField(state, optsIndex, "cull", true);
  options.depthTest = luaBoolField(state, optsIndex, "depth_test", true);
  options.depthWrite = luaBoolField(state, optsIndex, "depth_write", true);
  const std::string layerName = luaStringField(state, optsIndex, "layer", "");
  options.layer = runtime::parseModDrawLayer(layerName);
  options.entityId = luaIntField(state, optsIndex, "entity_id", 0);
  options.shaderEntity = luaStringField(state, optsIndex, "shader_entity", "");
  options.blockId = luaIntField(state, optsIndex, "block_id", 0);
  options.blockMeta = luaIntField(state, optsIndex, "block_meta", 0);
 }
 return options;
}
// World-space draw for a baked model; option parsing here, GL work in
// model::drawBakedModelWorld (a no-op returning false without the client
// renderer).
int luaModelDraw(lua_State* state) {
 LuaApi& api = luaApi();
 const int handle = luaIntArg(state, 1, 0);
 const int optsIndex = api.gettop(state) >= 2 && api.type(state, 2) == kLuaTTable ? 2 : 0;
 const model::WorldModelDraw options = readWorldModelDraw(state, optsIndex);
 api.pushboolean(state, model::drawBakedModelWorld(handle, options) ? 1 : 0);
 return 1;
}
// minecraft.model.draw_item(item_id, damage, opts) -> drew a real 3D model
// (custom Lua item/block model, or the vanilla/mod block-cube renderer).
// false for plain sprite items with no 3D shape; callers should fall back to
// their own flat-icon representation in that case.
int luaModelDrawItem(lua_State* state) {
 LuaApi& api = luaApi();
 const int itemId = luaIntArg(state, 1, 0);
 const int damage = luaIntArg(state, 2, 0);
 const int optsIndex = api.gettop(state) >= 3 && api.type(state, 3) == kLuaTTable ? 3 : 0;
 const model::WorldModelDraw options = readWorldModelDraw(state, optsIndex);
 const ItemStack stack(itemId, 1, damage);
 api.pushboolean(state, model::drawItemStackWorld(stack, options) ? 1 : 0);
 return 1;
}
// minecraft.model.item_bounds(item_id, damage) -> model-space bounds table for
// the same items draw_item draws a real model for, or nil otherwise.
int luaModelItemBounds(lua_State* state) {
 LuaApi& api = luaApi();
 const int itemId = luaIntArg(state, 1, 0);
 const int damage = luaIntArg(state, 2, 0);
 const ItemStack stack(itemId, 1, damage);
 model::BakedBounds bounds;
 if(!model::itemStackBounds(stack, bounds)) {
  api.pushnil(state);
  return 1;
 }
 api.createtable(state, 0, 6);
 setFields(state,
           "min_x",
           static_cast<double>(bounds.min[0]),
           "min_y",
           static_cast<double>(bounds.min[1]),
           "min_z",
           static_cast<double>(bounds.min[2]),
           "max_x",
           static_cast<double>(bounds.max[0]),
           "max_y",
           static_cast<double>(bounds.max[1]),
           "max_z",
           static_cast<double>(bounds.max[2]));
 return 1;
}
// Reads one {x,y,z,u,v} vertex from the table at vtxIndex into vertex.
void readBuildVertex(lua_State* state, int vtxIndex, model::BakedVertex& vertex) {
 vertex.x = luaFloatField(state, vtxIndex, "x", 0.0f);
 vertex.y = luaFloatField(state, vtxIndex, "y", 0.0f);
 vertex.z = luaFloatField(state, vtxIndex, "z", 0.0f);
 vertex.u = luaFloatField(state, vtxIndex, "u", 0.0f);
 vertex.v = luaFloatField(state, vtxIndex, "v", 0.0f);
}
// model.build{quads = {{texture?, r,g,b,a?, shade?, vertices = {v1,v2,v3,v4}}, ...},
// key?} -> handle. Generic model builder: assembles arbitrary colored/textured
// quads into a baked model. All voxel geometry (sprite sampling, interior-face
// culling, cube generation) is built on top of this in Lua.
int luaModelBuild(lua_State* state) {
 LuaApi& api = luaApi();
 if(api.type(state, 1) != kLuaTTable) {
  api.pushnil(state);
  api.pushstring(state, "model.build expects an options table");
  return 2;
 }
 const std::string key = luaStringField(state, 1, "key", "");
 if(!key.empty()) {
  if(const int cached = model::bakedModelHandleForKey(key)) {
   api.pushinteger(state, cached);
   return 1;
  }
 }
 api.getfield(state, 1, "quads");
 if(api.type(state, -1) != kLuaTTable) {
  api.settop(state, 1);
  api.pushnil(state);
  api.pushstring(state, "model.build requires a quads array");
  return 2;
 }
 const int quadsIndex = api.gettop(state);
 const std::size_t quadCount = api.rawlen(state, quadsIndex);
 auto baked = std::make_unique<model::BakedModel>();
 for(std::size_t qi = 1; qi <= quadCount; ++qi) {
  api.rawgeti(state, quadsIndex, static_cast<long long>(qi));
  const int quadIndex = api.gettop(state);
  if(api.type(state, quadIndex) == kLuaTTable) {
   const std::string texture = luaStringField(state, quadIndex, "texture", "");
   model::BakedQuad quad;
   quad.shade = std::clamp(luaFloatField(state, quadIndex, "shade", 1.0f), 0.0f, 1.0f);
   quad.red = std::clamp(luaFloatField(state, quadIndex, "r", 1.0f), 0.0f, 1.0f);
   quad.green = std::clamp(luaFloatField(state, quadIndex, "g", 1.0f), 0.0f, 1.0f);
   quad.blue = std::clamp(luaFloatField(state, quadIndex, "b", 1.0f), 0.0f, 1.0f);
   quad.alpha = std::clamp(luaFloatField(state, quadIndex, "a", 1.0f), 0.0f, 1.0f);
   api.getfield(state, quadIndex, "vertices");
   bool ok = false;
   if(api.type(state, -1) == kLuaTTable) {
    const int verticesIndex = api.gettop(state);
    if(api.rawlen(state, verticesIndex) >= 4) {
     ok = true;
     for(int vi = 0; vi < 4; ++vi) {
      api.rawgeti(state, verticesIndex, vi + 1);
      const int vtxIndex = api.gettop(state);
      if(api.type(state, vtxIndex) == kLuaTTable) {
       readBuildVertex(state, vtxIndex, quad.vertices[vi]);
      } else {
       ok = false;
      }
      api.settop(state, verticesIndex);
     }
    }
   }
   api.settop(state, quadIndex);
   if(ok) {
    model::detail::batchFor(*baked, texture).push_back(quad);
   }
  }
  api.settop(state, quadsIndex);
 }
 api.settop(state, 1);
 if(baked->batches.empty()) {
  api.pushnil(state);
  api.pushstring(state, "model.build requires at least one quad");
  return 2;
 }
 model::computeBakedBounds(*baked);
 // Keyless builds still need a unique registry key so their handles never alias.
 static std::atomic<std::uint64_t> anonCounter{0};
 const std::string storeKey = key.empty() ? ("\x01build#" + std::to_string(anonCounter.fetch_add(1))) : key;
 api.pushinteger(state, model::storeBakedModel(storeKey, std::move(baked)));
 return 1;
}
} // namespace
void installModelApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
 LuaApi& api = luaApi();
 api.createtable(state, 0, 10);
 bindModFunction(state, &mod, "load", luaModelLoad);
 bindModFunction(state, &mod, "place", luaModelPlace);
 bindModFunction(state, &mod, "clear", luaModelClear);
 bindFunctions(state,
               {
                   {"draw", luaModelDraw},
                   {"draw_item", luaModelDrawItem},
                   {"item_bounds", luaModelItemBounds},
                   {"build", luaModelBuild},
                   {"update", luaModelUpdate},
                   {"remove", luaModelRemove},
                   {"bounds", luaModelBounds},
               });
 api.setfield(state, -2, "model");
}
} // namespace net::minecraft::mod::runtime
