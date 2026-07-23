#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#endif
#include <algorithm>
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
[[nodiscard]] bool luaWorldIsOverworld(const World* world) {
 return world != nullptr && world->dimension != nullptr && !world->dimension->isNether &&
        !world->dimension->hasCeiling;
}
void setWorldContextFields(lua_State* state, const World* world) {
 setField(state, "has_world", world != nullptr);
 setField(state, "world_name", world != nullptr ? world->name() : std::string());
 setField(state, "is_overworld", luaWorldIsOverworld(world));
 setField(state, "mod_generation", world != nullptr && world->isLuaModGenerationEnabled());
}
void setLuaExecutionFields(lua_State* state, const World* world) {
 const bool remote = world != nullptr ? world->isRemote() : isClientBuild();
 setField(state, "remote", remote);
 setField(state, "side", remote ? "client" : "server");
}
void setEntityIdentityFields(lua_State* state, const net::minecraft::entity::Entity& entity) {
 setField(state, "entity_id", entity.id);
 setField(state, "entity_type", net::minecraft::entity::EntityRegistry::getId(entity));
}
void pushStringMap(lua_State* state, const std::unordered_map<std::string, std::string>& values) {
 LuaApi& api = luaApi();
 api.createtable(state, 0, static_cast<int>(values.size()));
 for(const auto& [key, value] : values) {
  api.pushlstring(state, value.data(), value.size());
  api.setfield(state, -2, key.c_str());
 }
}
void readStringMapField(lua_State* state,
                        int tableIndex,
                        const char* key,
                        std::unordered_map<std::string, std::string>& values) {
 LuaApi& api = luaApi();
 api.getfield(state, tableIndex, key);
 if(api.type(state, -1) != kLuaTTable) {
  pop(state, 1);
  return;
 }
 const int mapIndex = api.gettop(state);
 std::unordered_map<std::string, std::string> parsed;
 parsed.reserve(std::min<std::size_t>(api.rawlen(state, mapIndex), 256));
 api.pushnil(state);
 while(parsed.size() < 256 && api.next(state, mapIndex) != 0) {
  if(api.type(state, -2) == kLuaTString && api.type(state, -1) == kLuaTString) {
   const std::string mapKey = luaString(state, -2, "");
   const std::string mapValue = luaString(state, -1, "");
   if(!mapKey.empty() && mapKey.size() <= 128 && mapValue.size() <= 4096) {
    parsed[mapKey] = mapValue;
   }
  }
  pop(state, 1);
 }
 pop(state, 1);
 values = std::move(parsed);
}
void setClientTickFields(lua_State* state, const ClientTickEvent& event) {
 setField(state, "before", event.before);
 setField(state, "after_world", event.afterWorld);
 setField(state, "paused", event.paused);
 setField(state, "has_player", event.player != nullptr);
 setField(state, "has_world", event.world != nullptr);
 setWorldContextFields(state, event.world);
 double cameraY = 64.0;
 double playerY = 64.0;
 float playerFallDistance = 0.0f;
 bool playerOnGround = false;
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(event.client != nullptr && event.client->camera != nullptr) {
  const entity::Entity* camera = event.client->camera;
  cameraY = camera->lastTickY + (camera->y - camera->lastTickY);
 }
 if(event.player != nullptr) {
  playerY = event.player->y;
  playerFallDistance = event.player->fallDistance;
  playerOnGround = event.player->onGround;
 }
#endif
 setField(state, "camera_y", cameraY);
 setField(state, "player_y", playerY);
 setField(state, "player_fall_distance", playerFallDistance);
 setField(state, "player_on_ground", playerOnGround);
 if(event.world != nullptr) {
  setField(state, "world_time", static_cast<double>(event.world->getTime() % 24000ULL));
  setField(state, "is_night", net::minecraft::mod::lua::worldIsNight(event.world));
 } else {
  setField(state, "world_time", 0.0);
  setField(state, "is_night", false);
 }
}
bool isClientBuild() {
#ifdef MINECRAFT_NATIVE_EXPORTS
 return client::Minecraft::INSTANCE != nullptr;
#else
 return false;
#endif
}
bool isLocalPlayer(const entity::player::PlayerEntity* player) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 return player != nullptr && client::Minecraft::INSTANCE != nullptr &&
        player == static_cast<const entity::player::PlayerEntity*>(client::Minecraft::INSTANCE->player);
#else
 (void)player;
 return false;
#endif
}
bool isLuaModExecutionEnabled() {
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(isClientBuild() && !client::Minecraft::INSTANCE->options.modsEnabled) {
  return false;
 }
 // Gate on the active mod-context world (server interact sets the server world).
 // Never use only Minecraft::INSTANCE->world — that is ClientWorld (remote) in SP and
 // would suppress server-side Lua (entity spawn, inventory, etc.).
 World* world = contextOrClientWorld();
 if(world != nullptr && world->isRemote() && !world->isLuaModGenerationEnabled()) {
  return false;
 }
#endif
 return true;
}
} // namespace net::minecraft::mod::runtime
