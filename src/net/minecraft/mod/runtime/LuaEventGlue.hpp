#pragma once
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
namespace net::minecraft {
class World;
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
void setClientTickFields(lua_State* state, const ClientTickEvent& event);
void pushStringMap(lua_State* state, const std::unordered_map<std::string, std::string>& values);
void readStringMapField(lua_State* state, int tableIndex, const char* key,
                        std::unordered_map<std::string, std::string>& values);
bool isLuaModExecutionEnabled();
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
  api.createtable(state, 0, 12);
  fill(state);
  const int eventIndex = api.gettop(state);
  api.pushvalue(state, -2);
  api.pushvalue(state, -2);
  const int status = api.pcallk(state, 1, 1, 0, 0, nullptr);
  if(status != kLuaOk) {
    const char* error = api.tolstring(state, -1, nullptr);
    runtimeLog(mod->modId, "error", error != nullptr ? error : "Lua callback failed");
    api.settop(state, -4);
    return;
  }
  if(api.type(state, -1) != kLuaTTable) {
    api.settop(state, -2);
    api.pushvalue(state, eventIndex);
  }
  apply(state);
  api.settop(state, eventIndex - 2);
}
} // namespace net::minecraft::mod::runtime
