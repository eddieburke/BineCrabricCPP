#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::entity {
class Entity;
}
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::mod {
struct ClientTickEvent;
}
namespace net::minecraft::mod::runtime {
inline ModHost::LoadedLuaMod* currentLuaMod(lua_State* state) {
 using namespace net::minecraft::mod::lua;
 return static_cast<ModHost::LoadedLuaMod*>(luaApi().touserdata(state, luaUpvalueIndex(1)));
}
void setWorldContextFields(lua_State* state, const World* world);
void setLuaExecutionFields(lua_State* state, const World* world);
void setEntityIdentityFields(lua_State* state, const net::minecraft::entity::Entity& entity);
void setClientTickFields(lua_State* state, const ClientTickEvent& event);
[[nodiscard]] bool luaWorldIsOverworld(const World* world);
[[nodiscard]] bool luaWorldIsOverworld(const World* world);
void pushStringMap(lua_State* state, const std::unordered_map<std::string, std::string>& values);
void readStringMapField(lua_State* state,
                        int tableIndex,
                        const char* key,
                        std::unordered_map<std::string, std::string>& values);
bool isLuaModExecutionEnabled();
// True when this is the client build (MINECRAFT_NATIVE_EXPORTS) and Minecraft::INSTANCE
// is live. The one place that answers "are we the client process" — binding/event code
// should call this instead of open-coding #ifdef MINECRAFT_NATIVE_EXPORTS + INSTANCE
// checks, so the client-detection logic has a single definition to fix or extend.
bool isClientBuild();
// True when `player` is this client's own local player. This is the right gate for
// client-side reactions (opening screens, HUD): world remoteness alone says nothing
// about who triggered the event, and is false for the singleplayer world.
bool isLocalPlayer(const entity::player::PlayerEntity* player);
template <typename Fill, typename Apply>
void callLuaEvent(const std::shared_ptr<ModHost::LoadedLuaMod>& mod, int ref, Fill fill, Apply apply) {
 if(!isLuaModExecutionEnabled()) {
  return;
 }
 using namespace net::minecraft::mod::lua;
 LuaApi& api = luaApi();
 if(!api.ready() || mod == nullptr) {
  return;
 }
 const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
 if(!mod->active || mod->state == nullptr) {
  return;
 }
 auto* state = static_cast<lua_State*>(mod->state);
 api.rawgeti(state, kLuaRegistryIndex, ref);
 if(api.type(state, -1) != kLuaTFunction) {
  api.settop(state, -2);
  return;
 }
 api.createtable(state, 0, 24);
 const int tableIndex = api.gettop(state);
 fill(state);
 const int status = api.pcallk(state, 1, 1, 0, 0, nullptr);
 if(status != kLuaOk) {
  const char* error = api.tolstring(state, -1, nullptr);
  runtimeLog(mod->modId, "error", error != nullptr ? error : "Lua callback failed");
  api.settop(state, tableIndex - 2);
  return;
 }
 apply(state);
 api.settop(state, tableIndex - 2);
}
} // namespace net::minecraft::mod::runtime
