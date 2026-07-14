#include "net/minecraft/mod/runtime/LuaWorldBindings.hpp"
#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#endif
#include <algorithm>
#include <cmath>
#include <string>
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
int luaChunkSetBlock(lua_State* state) {
  LuaApi& api = luaApi();
  const int argOffset = 1;
  if(!LuaChunkContext::hasActiveChunk() || api.gettop(state) < 4 + argOffset) {
    return 0;
  }
  const int localX = luaIntArg(state, 1 + argOffset);
  const int y = luaIntArg(state, 2 + argOffset);
  const int localZ = luaIntArg(state, 3 + argOffset);
  const int blockId = luaIntArg(state, 4 + argOffset);
  api.pushboolean(state, LuaChunkContext::setBlock(localX, y, localZ, blockId) ? 1 : 0);
  return 1;
}
int luaChunkFillBlock(lua_State* state) {
  LuaApi& api = luaApi();
  const int argOffset = 1;
  if(!LuaChunkContext::hasActiveChunk() || api.gettop(state) < 7 + argOffset) {
    api.pushinteger(state, 0);
    return 1;
  }
  int x1 = luaIntArg(state, 1 + argOffset);
  int y1 = luaIntArg(state, 2 + argOffset);
  int z1 = luaIntArg(state, 3 + argOffset);
  int x2 = luaIntArg(state, 4 + argOffset);
  int y2 = luaIntArg(state, 5 + argOffset);
  int z2 = luaIntArg(state, 6 + argOffset);
  const int blockId = luaIntArg(state, 7 + argOffset);
  if(x1 > x2) {
    std::swap(x1, x2);
  }
  if(y1 > y2) {
    std::swap(y1, y2);
  }
  if(z1 > z2) {
    std::swap(z1, z2);
  }
  x1 = std::clamp(x1, 0, Chunk::width - 1);
  x2 = std::clamp(x2, 0, Chunk::width - 1);
  y1 = std::clamp(y1, 0, Chunk::height - 1);
  y2 = std::clamp(y2, 0, Chunk::height - 1);
  z1 = std::clamp(z1, 0, Chunk::depth - 1);
  z2 = std::clamp(z2, 0, Chunk::depth - 1);
  int changed = 0;
  for(int y = y1; y <= y2; ++y) {
    for(int z = z1; z <= z2; ++z) {
      for(int x = x1; x <= x2; ++x) {
        if(LuaChunkContext::setBlock(x, y, z, blockId)) {
          ++changed;
        }
      }
    }
  }
  api.pushinteger(state, changed);
  return 1;
}
int luaChunkGetBlock(lua_State* state) {
  LuaApi& api = luaApi();
  const int argOffset = 1;
  if(!LuaChunkContext::hasActiveChunk() || api.gettop(state) < 3 + argOffset) {
    api.pushinteger(state, 0);
    return 1;
  }
  const int localX = luaIntArg(state, 1 + argOffset);
  const int y = luaIntArg(state, 2 + argOffset);
  const int localZ = luaIntArg(state, 3 + argOffset);
  api.pushinteger(state, LuaChunkContext::getBlock(localX, y, localZ));
  return 1;
}
int luaChunkGetHeight(lua_State* state) {
  LuaApi& api = luaApi();
  const int argOffset = 1;
  if(!LuaChunkContext::hasActiveChunk() || api.gettop(state) < 2 + argOffset) {
    api.pushinteger(state, 0);
    return 1;
  }
  const int localX = luaIntArg(state, 1 + argOffset);
  const int localZ = luaIntArg(state, 2 + argOffset);
  api.pushinteger(state, LuaChunkContext::getHeight(localX, localZ));
  return 1;
}
[[nodiscard]] World* luaActiveWorld() {
  World* world = LuaChunkContext::activeWorld();
  if(world == nullptr) {
    world = contextOrClientWorld();
  }
  return world;
}
int luaWorldBlockId(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.type(state, 1) != kLuaTString) {
    api.pushinteger(state, 0);
    return 1;
  }
  api.pushinteger(state, blockIdFromName(luaString(state, 1, "").c_str()));
  return 1;
}
int luaWorldRandom(lua_State* state) {
  LuaApi& api = luaApi();
  int bound = 1000;
  if(api.gettop(state) >= 1 && api.type(state, 1) == kLuaTNumber) {
    bound = luaIntArg(state, 1, bound);
  }
  World* world = luaActiveWorld();
  api.pushinteger(state, worldRandomInt(world, bound));
  return 1;
}
int luaWorldIsNight(lua_State* state) {
  (void)state;
  LuaApi& api = luaApi();
  World* world = luaActiveWorld();
  api.pushboolean(state, worldIsNight(world) ? 1 : 0);
  return 1;
}
int luaWorldGetTime(lua_State* state) {
  LuaApi& api = luaApi();
  World* world = luaActiveWorld();
  api.pushinteger(state, world != nullptr ? static_cast<std::int64_t>(world->getTime()) : 0);
  return 1;
}
int luaWorldTopSolidY(lua_State* state) {
  LuaApi& api = luaApi();
  const int x = luaIntArg(state, 1);
  const int z = luaIntArg(state, 2);
  World* world = luaActiveWorld();
  if(world == nullptr) {
    api.pushinteger(state, -1);
    return 1;
  }
  api.pushinteger(state, world->getTopSolidBlockY(x, z));
  return 1;
}
int luaWorldPlayer(lua_State* state) {
  LuaApi& api = luaApi();
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  if(entity::player::PlayerEntity* player = activeModPlayer(); player != nullptr) {
    x = player->x;
    y = player->y;
    z = player->z;
  } else if(!readPlayerPosition(x, y, z)) {
    api.pushnil(state);
    return 1;
  }
  api.createtable(state, 0, 3);
  setField(state, "x", x);
  setField(state, "y", y);
  setField(state, "z", z);
  return 1;
}
int luaWorldSpawnEntity(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 2 || api.type(state, 1) != kLuaTString) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string entityId = luaString(state, 1, "");
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  if(api.type(state, 2) == kLuaTTable) {
    x = luaFloatField(state, 2, "x", luaFloatAt(state, 2, 1, 0.0f));
    y = luaFloatField(state, 2, "y", luaFloatAt(state, 2, 2, 64.0f));
    z = luaFloatField(state, 2, "z", luaFloatAt(state, 2, 3, 0.0f));
  } else {
    x = luaDoubleArg(state, 2);
    y = api.gettop(state) >= 3 ? luaDoubleArg(state, 3) : 64.0;
    z = api.gettop(state) >= 4 ? luaDoubleArg(state, 4) : 0.0;
  }
  World* world = luaActiveWorld();
  api.pushboolean(state, world != nullptr && !world->isRemote() && spawnEntityByName(world, entityId.c_str(), x, y, z) ? 1 : 0);
  return 1;
}
int luaWorldCountEntities(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.type(state, 1) != kLuaTString) {
    api.pushinteger(state, 0);
    return 1;
  }
  World* world = luaActiveWorld();
  api.pushinteger(state, countEntitiesByName(world, luaString(state, 1, "").c_str()));
  return 1;
}
int luaWorldGetBlock(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 3) {
    api.pushinteger(state, 0);
    return 1;
  }
  const int x = luaIntArg(state, 1);
  const int y = luaIntArg(state, 2);
  const int z = luaIntArg(state, 3);
  World* world = luaActiveWorld();
  api.pushinteger(state, getBlockIdAt(world, x, y, z));
  return 1;
}
int luaWorldSetTime(lua_State* state) {
  LuaApi& api = luaApi();
  int isNumber = 0;
  const long long tick = api.tointegerx(state, 1, &isNumber);
  if(isNumber == 0) {
    api.pushboolean(state, 0);
    return 1;
  }
  World* world = luaActiveWorld();
  if(world != nullptr && !world->isRemote()) {
    world->synchronizeTimeAndUpdates(static_cast<std::uint64_t>(tick));
    api.pushboolean(state, 1);
    return 1;
  }
  api.pushboolean(state, 0);
  return 1;
}
int luaParticlesSpawn(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.type(state, 1) != kLuaTTable) {
    api.pushboolean(state, 0);
    return 1;
  }
  const int tableIndex = 1;
  const double x = luaFloatField(state, tableIndex, "x", 0.0f);
  const double y = luaFloatField(state, tableIndex, "y", 64.0f);
  const double z = luaFloatField(state, tableIndex, "z", 0.0f);
  const double vx = luaFloatField(state, tableIndex, "vx", 0.0f);
  const double vy = luaFloatField(state, tableIndex, "vy", 0.0f);
  const double vz = luaFloatField(state, tableIndex, "vz", 0.0f);
  const float scale = luaFloatField(state, tableIndex, "scale", 4.0f);
  const float red = luaFloatField(state, tableIndex, "r", 1.0f);
  const float green = luaFloatField(state, tableIndex, "g", 1.0f);
  const float blue = luaFloatField(state, tableIndex, "b", 1.0f);
  const int maxAge = luaIntField(state, tableIndex, "max_age", 40);
  const float gravity = luaFloatField(state, tableIndex, "gravity", 0.04f);
  api.pushboolean(state, spawnClientParticle(x, y, z, vx, vy, vz, scale, red, green, blue, maxAge, gravity) ? 1 : 0);
  return 1;
}
int luaItemIds(lua_State* state) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 256);
  int index = 1;
  for(const Item* item : Item::ITEMS) {
    if(item == nullptr) {
      continue;
    }
    api.pushinteger(state, item->id);
    api.rawseti(state, -2, index++);
  }
  return 1;
}
} // namespace
void pushChunkObject(lua_State* state) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 4);
  bindFunctions(state,
                {
                    {"set_block", luaChunkSetBlock},
                    {"fill", luaChunkFillBlock},
                    {"get_block", luaChunkGetBlock},
                    {"get_height", luaChunkGetHeight},
                });
}
void installWorldApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
  LuaApi& api = luaApi();
  (void)mod;
  pushFunctionTable(state,
                    {
                        {"block_id", luaWorldBlockId},
                        {"get_block", luaWorldGetBlock},
                        {"random", luaWorldRandom},
                        {"is_night", luaWorldIsNight},
                        {"get_time", luaWorldGetTime},
                        {"get_top_y", luaWorldTopSolidY},
                        {"player", luaWorldPlayer},
                        {"spawn_entity", luaWorldSpawnEntity},
                        {"count_entities", luaWorldCountEntities},
                        {"set_time", luaWorldSetTime},
                    });
  api.setfield(state, -2, "world");
  pushFunctionTable(state, {{"spawn", luaParticlesSpawn}});
  api.setfield(state, -2, "particles");
  pushFunctionTable(state, {{"ids", luaItemIds}});
  api.setfield(state, -2, "items");
}
} // namespace net::minecraft::mod::runtime
