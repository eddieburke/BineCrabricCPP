#include "net/minecraft/mod/runtime/LuaBlockEntityBindings.hpp"
#include <cmath>
#include <map>
#include <string>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/mod/lua/LuaModEntity.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaNbtCodec.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
struct TileEntityAnim {
  std::int64_t tick = 0;
  float speed = 1.0f;
};
std::map<const block::entity::BlockEntity*, TileEntityAnim>& animRegistry() {
  static std::map<const block::entity::BlockEntity*, TileEntityAnim> registry;
  return registry;
}
block::entity::BlockEntity* teSelf(lua_State* state, int index) {
  LuaApi& api = luaApi();
  api.getfield(state, index, "__entity");
  auto* entity = static_cast<block::entity::BlockEntity*>(api.touserdata(state, -1));
  pop(state, 1);
  return entity;
}
int teGetId(lua_State* state) {
  LuaApi& api = luaApi();
  const auto* entity = teSelf(state, 1);
  if(entity == nullptr) {
    api.pushnil(state);
    return 1;
  }
  api.pushstring(state, entity->id().c_str());
  return 1;
}
int teGetBlockId(lua_State* state) {
  LuaApi& api = luaApi();
  const auto* entity = teSelf(state, 1);
  if(entity == nullptr || entity->world == nullptr) {
    api.pushinteger(state, 0);
    return 1;
  }
  api.pushinteger(state, entity->world->getBlockId(entity->x, entity->y, entity->z));
  return 1;
}
int teGetBlockMeta(lua_State* state) {
  LuaApi& api = luaApi();
  const auto* entity = teSelf(state, 1);
  if(entity == nullptr || entity->world == nullptr) {
    api.pushinteger(state, 0);
    return 1;
  }
  api.pushinteger(state, entity->world->getBlockMeta(entity->x, entity->y, entity->z));
  return 1;
}
int teIsRemoved(lua_State* state) {
  LuaApi& api = luaApi();
  const auto* entity = teSelf(state, 1);
  api.pushboolean(state, entity != nullptr && entity->isRemoved() ? 1 : 0);
  return 1;
}
int teMarkDirty(lua_State* state) {
  auto* entity = teSelf(state, 1);
  if(entity != nullptr && entity->world != nullptr && !entity->world->isRemote()) {
    entity->markDirty();
  }
  return 0;
}
int teDistanceFrom(lua_State* state) {
  LuaApi& api = luaApi();
  const auto* entity = teSelf(state, 1);
  if(entity == nullptr) {
    api.pushnil(state);
    return 1;
  }
  int isNumber = 0;
  const double tx = api.tonumberx(state, 2, &isNumber);
  const double ty = api.tonumberx(state, 3, &isNumber);
  const double tz = api.tonumberx(state, 4, &isNumber);
  api.pushnumber(state, entity->distanceFrom(tx, ty, tz));
  return 1;
}
int teGetWorldTime(lua_State* state) {
  LuaApi& api = luaApi();
  const auto* entity = teSelf(state, 1);
  if(entity == nullptr || entity->world == nullptr) {
    api.pushnumber(state, 0.0);
    return 1;
  }
  api.pushnumber(state, static_cast<double>(entity->world->getTime()));
  return 1;
}
int teGetData(lua_State* state) {
  LuaApi& api = luaApi();
  const auto* entity = teSelf(state, 1);
  if(entity == nullptr) {
    api.pushnil(state);
    return 1;
  }
  const auto* modEntity = dynamic_cast<const LuaModBlockEntity*>(entity);
  if(modEntity == nullptr) {
    api.pushnil(state);
    return 1;
  }
  pushNbtValue(state, modEntity->data().storage());
  return 1;
}
int teSetData(lua_State* state) {
  LuaApi& api = luaApi();
  auto* entity = teSelf(state, 1);
  if(entity == nullptr || entity->world == nullptr || entity->world->isRemote() || api.type(state, 2) != kLuaTTable) {
    return 0;
  }
  auto* modEntity = dynamic_cast<LuaModBlockEntity*>(entity);
  if(modEntity == nullptr) {
    return 0;
  }
  modEntity->data() = NbtCompound(luaValueToNbt(state, 2));
  modEntity->markDirty();
  return 0;
}
int teGetAnimationFrame(lua_State* state) {
  LuaApi& api = luaApi();
  const auto* entity = teSelf(state, 1);
  api.pushinteger(state, tileEntityAnimationFrame(entity));
  return 1;
}
int teSetAnimationSpeed(lua_State* state) {
  LuaApi& api = luaApi();
  auto* entity = teSelf(state, 1);
  if(entity != nullptr && api.gettop(state) >= 2) {
    int isNumber = 0;
    const double speed = api.tonumberx(state, 2, &isNumber);
    if(std::isfinite(speed)) {
      setTileEntityAnimationSpeed(entity, static_cast<float>(speed));
    }
  }
  return 0;
}
} // namespace
void pushTileEntityHandle(lua_State* state, block::entity::BlockEntity* entity) {
  LuaApi& api = luaApi();
  if(entity == nullptr) {
    api.pushnil(state);
    return;
  }
  api.createtable(state, 0, 16);
  setField(state, "x", entity->x);
  setField(state, "y", entity->y);
  setField(state, "z", entity->z);
  api.pushlightuserdata(state, entity);
  api.setfield(state, -2, "__entity");
  bindFunctions(state,
                {
                    {"get_id", teGetId},
                    {"get_block_id", teGetBlockId},
                    {"get_block_meta", teGetBlockMeta},
                    {"is_removed", teIsRemoved},
                    {"mark_dirty", teMarkDirty},
                    {"distance_from", teDistanceFrom},
                    {"get_world_time", teGetWorldTime},
                    {"get_data", teGetData},
                    {"set_data", teSetData},
                    {"get_animation_frame", teGetAnimationFrame},
                    {"set_animation_speed", teSetAnimationSpeed},
                });
}
namespace {
int luaTileEntitiesList(lua_State* state) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 16);
  World* world = contextOrClientWorld();
  if(world == nullptr) {
    return 1;
  }
  const std::string filter =
      api.gettop(state) >= 1 && api.type(state, 1) == kLuaTString ? luaString(state, 1, "") : std::string();
  int count = 0;
  for(block::entity::BlockEntity* entity : world->blockEntities) {
    if(entity == nullptr) {
      continue;
    }
    if(!filter.empty() && entity->id() != filter) {
      continue;
    }
    pushTileEntityHandle(state, entity);
    api.rawseti(state, -2, ++count);
  }
  return 1;
}
int luaTileEntitiesGet(lua_State* state) {
  LuaApi& api = luaApi();
  World* world = contextOrClientWorld();
  if(world == nullptr || api.gettop(state) < 3) {
    api.pushnil(state);
    return 1;
  }
  int isNumber = 0;
  const int x = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  const int y = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  const int z = static_cast<int>(api.tonumberx(state, 3, &isNumber));
  pushTileEntityHandle(state, world->getBlockEntity(x, y, z));
  return 1;
}
int luaTileEntitiesCount(lua_State* state) {
  LuaApi& api = luaApi();
  World* world = contextOrClientWorld();
  if(world == nullptr) {
    api.pushinteger(state, 0);
    return 1;
  }
  const std::string filter =
      api.gettop(state) >= 1 && api.type(state, 1) == kLuaTString ? luaString(state, 1, "") : std::string();
  int count = 0;
  for(block::entity::BlockEntity* entity : world->blockEntities) {
    if(entity == nullptr) {
      continue;
    }
    if(!filter.empty() && entity->id() != filter) {
      continue;
    }
    ++count;
  }
  api.pushinteger(state, count);
  return 1;
}
} // namespace
int tileEntityAnimationFrame(const block::entity::BlockEntity* entity) {
  if(entity == nullptr) {
    return 0;
  }
  const auto& registry = animRegistry();
  auto it = registry.find(entity);
  if(it == registry.end()) {
    return 0;
  }
  return static_cast<int>(it->second.tick * it->second.speed);
}
float tileEntityAnimationSpeed(const block::entity::BlockEntity* entity) {
  if(entity == nullptr) {
    return 1.0f;
  }
  const auto& registry = animRegistry();
  auto it = registry.find(entity);
  return it == registry.end() ? 1.0f : it->second.speed;
}
std::int64_t tileEntityAnimTick(const block::entity::BlockEntity* entity) {
  if(entity == nullptr) {
    return 0;
  }
  const auto& registry = animRegistry();
  auto it = registry.find(entity);
  return it == registry.end() ? 0 : it->second.tick;
}
void setTileEntityAnimationSpeed(const block::entity::BlockEntity* entity, float speed) {
  if(entity == nullptr) {
    return;
  }
  animRegistry()[entity].speed = speed > 0.0f ? speed : 0.0f;
}
void tickTileEntityAnimation(const block::entity::BlockEntity* entity) {
  if(entity == nullptr) {
    return;
  }
  animRegistry()[entity].tick += 1;
}
void clearTileEntityAnimation(const block::entity::BlockEntity* entity) {
  if(entity != nullptr) {
    animRegistry().erase(entity);
  }
}
void installTileEntityApi(lua_State* state) {
  LuaApi& api = luaApi();
  pushFunctionTable(state,
                    {
                        {"list", luaTileEntitiesList},
                        {"get", luaTileEntitiesGet},
                        {"count", luaTileEntitiesCount},
                    });
  api.setfield(state, -2, "tile_entities");
}
} // namespace net::minecraft::mod::runtime
