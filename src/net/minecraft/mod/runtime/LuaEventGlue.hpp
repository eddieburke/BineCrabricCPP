#pragma once
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
namespace net::minecraft {
class ItemStack;
class World;
} // namespace net::minecraft
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
 if(auto* mod = static_cast<ModHost::LoadedLuaMod*>(luaApi().touserdata(state, luaUpvalueIndex(1)))) {
  return mod;
 }
 for(const auto& mod : loadedLuaMods()) {
  if(mod != nullptr && mod->state == state) {
   return mod.get();
  }
 }
 return nullptr;
}
void setWorldContextFields(lua_State* state, const World* world);
void setLuaExecutionFields(lua_State* state, const World* world);
void setEntityIdentityFields(lua_State* state, const net::minecraft::entity::Entity& entity);
void pushItemStackFields(lua_State* state, const ItemStack& stack);
void setClientTickFields(lua_State* state, const ClientTickEvent& event);
[[nodiscard]] bool luaWorldIsOverworld(const World* world);
[[nodiscard]] bool luaWorldIsOverworld(const World* world);
void pushStringMap(lua_State* state, const std::unordered_map<std::string, std::string>& values);
void readStringMapField(lua_State* state,
                        int tableIndex,
                        const char* key,
                        std::unordered_map<std::string, std::string>& values);
bool isLuaModExecutionEnabled();
bool isClientBuild();
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
