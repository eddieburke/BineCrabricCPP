#include "net/minecraft/mod/runtime/LuaModelBindings.hpp"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/model/BakedModel.hpp"
#include "net/minecraft/mod/model/ModelDraw.hpp"
#include "net/minecraft/mod/model/ModelInstances.hpp"
#include "net/minecraft/mod/model/ModelRegistry.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
namespace net::minecraft::mod::runtime {
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
  setFields(state, "min_x", static_cast<double>(b.min[0]), "min_y", static_cast<double>(b.min[1]), "min_z",
            static_cast<double>(b.min[2]), "max_x", static_cast<double>(b.max[0]), "max_y",
            static_cast<double>(b.max[1]), "max_z", static_cast<double>(b.max[2]));
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
    options.blend = luaBoolField(state, optsIndex, "blend", true);
    options.cull = luaBoolField(state, optsIndex, "cull", false);
    options.depthTest = luaBoolField(state, optsIndex, "depth_test", true);
    options.depthWrite = luaBoolField(state, optsIndex, "depth_write", true);
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
  setFields(state, "min_x", static_cast<double>(bounds.min[0]), "min_y", static_cast<double>(bounds.min[1]), "min_z",
            static_cast<double>(bounds.min[2]), "max_x", static_cast<double>(bounds.max[0]), "max_y",
            static_cast<double>(bounds.max[1]), "max_z", static_cast<double>(bounds.max[2]));
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
  const auto batchFor = [&](const std::string& texture) -> std::vector<model::BakedQuad>& {
    for(model::BakedTextureBatch& batch : baked->batches) {
      if(batch.texturePath == texture) {
        return batch.quads;
      }
    }
    model::BakedTextureBatch& batch = baked->batches.emplace_back();
    batch.texturePath = texture;
    return batch.quads;
  };
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
        batchFor(texture).push_back(quad);
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
  const std::string storeKey =
      key.empty() ? ("\x01build#" + std::to_string(anonCounter.fetch_add(1))) : key;
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
  bindFunctions(state, {
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
