#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaBindings.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
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
 LuaArgs args(state);
 const int argOffset = 1;
 int localX = 0;
 int y = 0;
 int localZ = 0;
 int blockId = 0;
 if(!LuaChunkContext::hasActiveChunk() || !args.integer(1 + argOffset, localX) ||
    !args.integer(2 + argOffset, y) || !args.integer(3 + argOffset, localZ) ||
    !args.integer(4 + argOffset, blockId)) {
  return 0;
 }
 api.pushboolean(state, LuaChunkContext::setBlock(localX, y, localZ, blockId) ? 1 : 0);
 return 1;
}
int luaChunkFillBlock(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 const int argOffset = 1;
 int x1 = 0;
 int y1 = 0;
 int z1 = 0;
 int x2 = 0;
 int y2 = 0;
 int z2 = 0;
 int blockId = 0;
 if(!LuaChunkContext::hasActiveChunk() || !args.integer(1 + argOffset, x1) ||
    !args.integer(2 + argOffset, y1) || !args.integer(3 + argOffset, z1) ||
    !args.integer(4 + argOffset, x2) || !args.integer(5 + argOffset, y2) ||
    !args.integer(6 + argOffset, z2) || !args.integer(7 + argOffset, blockId)) {
  api.pushinteger(state, 0);
  return 1;
 }
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
 LuaArgs args(state);
 const int argOffset = 1;
 int localX = 0;
 int y = 0;
 int localZ = 0;
 if(!LuaChunkContext::hasActiveChunk() || !args.integer(1 + argOffset, localX) ||
    !args.integer(2 + argOffset, y) || !args.integer(3 + argOffset, localZ)) {
  api.pushinteger(state, 0);
  return 1;
 }
 api.pushinteger(state, LuaChunkContext::getBlock(localX, y, localZ));
 return 1;
}
int luaChunkGetHeight(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 const int argOffset = 1;
 int localX = 0;
 int localZ = 0;
 if(!LuaChunkContext::hasActiveChunk() || !args.integer(1 + argOffset, localX) ||
    !args.integer(2 + argOffset, localZ)) {
  api.pushinteger(state, 0);
  return 1;
 }
 api.pushinteger(state, LuaChunkContext::getHeight(localX, localZ));
 return 1;
}
[[nodiscard]] World* luaActiveWorld() {
 World* world = LuaChunkContext::activeWorld();
 if(world == nullptr) {
  world = activeModWorld();
 }
 return world;
}
int luaWorldBlockId(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 std::string name;
 if(!args.string(1, name)) {
  api.pushinteger(state, 0);
  return 1;
 }
 api.pushinteger(state, blockIdFromName(name.c_str()));
 return 1;
}
int luaWorldRandom(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 int bound = 1000;
 if(args.count() >= 1 && !args.integer(1, bound)) {
  api.pushinteger(state, 0);
  return 1;
 }
 World* world = luaActiveWorld();
 api.pushinteger(state, worldRandomInt(world, bound));
 return 1;
}
int luaWorldIsNight(lua_State* state) {
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
 LuaArgs args(state);
 int x = 0;
 int z = 0;
 if(!args.integer(1, x) || !args.integer(2, z)) {
  api.pushinteger(state, -1);
  return 1;
 }
 World* world = luaActiveWorld();
 if(world == nullptr) {
  api.pushinteger(state, -1);
  return 1;
 }
 api.pushinteger(state, world->getTopSolidBlockY(x, z));
 return 1;
}
int luaWorldGetHeightmap(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 int originX = 0;
 int originZ = 0;
 int width = 128;
 int height = 128;
 if(!args.integer(1, originX) || !args.integer(2, originZ) ||
    (args.count() >= 3 && !args.integer(3, width)) ||
    (args.count() >= 4 && !args.integer(4, height))) {
  api.pushnil(state);
  return 1;
 }
 width = std::clamp(width, 1, 512);
 height = std::clamp(height, 1, 512);
 World* world = luaActiveWorld();
 if(world == nullptr) {
  api.pushnil(state);
  return 1;
 }
 api.createtable(state, width * height, 0);
 long long index = 1;
 for(int row = 0; row < height; ++row) {
  for(int column = 0; column < width; ++column) {
   const int worldX = originX + column;
   const int worldZ = originZ + row;
   int opaqueHeight = 0;
   int transparentHeight = 0;
   int transparentOpacity = 0;
   if(const Chunk* chunk = world->getChunkIfLoaded(worldX, worldZ); chunk != nullptr) {
    const int localX = MathHelper::floorMod(worldX, Chunk::width);
    const int localZ = MathHelper::floorMod(worldZ, Chunk::depth);
    for(int worldY = Chunk::height - 1; worldY >= 0; --worldY) {
     const int blockId = chunk->getBlockId(localX, worldY, localZ);
     if(blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
      continue;
     }
     const Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
     if(block == nullptr) {
      continue;
     }
     const int top = worldY + 1;
     if(block->isOpaque()) {
      opaqueHeight = std::max(opaqueHeight, top);
      continue;
     }
     const int opacity =
         std::clamp(Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(blockId)], 0, 255);
     if(opacity > 0) {
      transparentHeight = std::max(transparentHeight, top);
      transparentOpacity = std::max(transparentOpacity, opacity);
     }
    }
   }
   const std::uint32_t pixel = 0xFF000000U | (static_cast<std::uint32_t>(transparentHeight) << 16U) |
                               (static_cast<std::uint32_t>(transparentOpacity) << 8U) |
                               static_cast<std::uint32_t>(opaqueHeight);
   api.pushinteger(state, static_cast<long long>(pixel));
   api.rawseti(state, -2, index++);
  }
 }
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
 LuaArgs args(state);
 std::string entityId;
 if(!args.string(1, entityId) || args.count() < 2) {
  api.pushboolean(state, 0);
  return 1;
 }
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 if(args.table(2)) {
  x = luaDoubleField(state, 2, "x", luaFloatAt(state, 2, 1, 0.0f));
  y = luaDoubleField(state, 2, "y", luaFloatAt(state, 2, 2, 64.0f));
  z = luaDoubleField(state, 2, "z", luaFloatAt(state, 2, 3, 0.0f));
 } else if(!args.number(2, x) || !args.optionalNumber(3, y) || !args.optionalNumber(4, z)) {
  api.pushboolean(state, 0);
  return 1;
 }
 World* world = luaActiveWorld();
 api.pushboolean(
     state, world != nullptr && !world->isRemote() && spawnEntityByName(world, entityId.c_str(), x, y, z) ? 1 : 0);
 return 1;
}
int luaWorldCountEntities(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 std::string name;
 if(!args.string(1, name)) {
  api.pushinteger(state, 0);
  return 1;
 }
 World* world = luaActiveWorld();
 api.pushinteger(state, countEntitiesByName(world, name.c_str()));
 return 1;
}
int luaWorldGetBlock(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 int x = 0;
 int y = 0;
 int z = 0;
 if(!args.integer(1, x) || !args.integer(2, y) || !args.integer(3, z)) {
  api.pushinteger(state, 0);
  return 1;
 }
 World* world = luaActiveWorld();
 api.pushinteger(state, getBlockIdAt(world, x, y, z));
 return 1;
}
int luaWorldGetBlockMeta(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 int x = 0;
 int y = 0;
 int z = 0;
 if(!args.integer(1, x) || !args.integer(2, y) || !args.integer(3, z)) {
  api.pushinteger(state, 0);
  return 1;
 }
 World* world = luaActiveWorld();
 api.pushinteger(state, world != nullptr ? world->getBlockMeta(x, y, z) : 0);
 return 1;
}
int luaWorldSetTime(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 int isNumber = 0;
 const long long tick = api.tointegerx(state, 1, &isNumber);
 if(args.count() < 1 || isNumber == 0) {
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
 LuaArgs args(state);
 if(!args.table(1)) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int tableIndex = 1;
 const double x = luaDoubleField(state, tableIndex, "x", 0.0);
 const double y = luaDoubleField(state, tableIndex, "y", 64.0);
 const double z = luaDoubleField(state, tableIndex, "z", 0.0);
 const double vx = luaDoubleField(state, tableIndex, "vx", 0.0);
 const double vy = luaDoubleField(state, tableIndex, "vy", 0.0);
 const double vz = luaDoubleField(state, tableIndex, "vz", 0.0);
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
                        {"get_block_meta", luaWorldGetBlockMeta},
                        {"random", luaWorldRandom},
                       {"is_night", luaWorldIsNight},
                       {"get_time", luaWorldGetTime},
                       {"get_top_y", luaWorldTopSolidY},
                       {"get_heightmap", luaWorldGetHeightmap},
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
