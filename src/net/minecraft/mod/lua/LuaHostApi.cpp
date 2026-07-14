#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include <cstring>
#include <filesystem>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
namespace net::minecraft::mod::lua {
namespace {
#ifdef _WIN32
template <typename T>
void bindLuaSymbol(void* module, const char* name, T& out) {
  FARPROC raw = GetProcAddress(static_cast<HMODULE>(module), name);
  static_assert(sizeof(raw) == sizeof(out));
  std::memcpy(&out, &raw, sizeof(out));
}
#endif
} // namespace
int luaUpvalueIndex(int i) {
  return kLuaRegistryIndex - i;
}
bool LuaApi::ready() const {
  return newstate != nullptr && openlibs != nullptr && close != nullptr && loadfilex != nullptr &&
         loadbufferx != nullptr && pcallk != nullptr && gettop != nullptr && checkstack != nullptr &&
         settop != nullptr && type != nullptr &&
         tolstring != nullptr && tonumberx != nullptr && tointegerx != nullptr && toboolean != nullptr &&
         touserdata != nullptr && pushnil != nullptr && pushboolean != nullptr && pushnumber != nullptr &&
         pushinteger != nullptr && pushstring != nullptr && pushlstring != nullptr && pushlightuserdata != nullptr &&
         pushvalue != nullptr && pushcclosure != nullptr && createtable != nullptr && setfield != nullptr &&
         getfield != nullptr && setglobal != nullptr && ref != nullptr && unref != nullptr && rawgeti != nullptr &&
         rawseti != nullptr && next != nullptr && settable != nullptr && rawlen != nullptr;
}
LuaApi& luaApi() {
  static LuaApi api;
  static bool attempted = false;
  if(attempted) {
    return api;
  }
  attempted = true;
#ifdef _WIN32
  api.module = LoadLibraryW(L"lua54.dll");
  if(api.module == nullptr) {
    std::filesystem::path bundled =
        std::filesystem::path(MINECRAFT_NATIVE_SOURCE_DIR) / "toolchain" / "mingw64" / "bin" / "lua54.dll";
    api.module = LoadLibraryW(bundled.wstring().c_str());
  }
  if(api.module == nullptr) {
    runtimeLog("", "error", "lua54.dll not found; Lua mods cannot load");
    return api;
  }
  bindLuaSymbol(api.module, "luaL_newstate", api.newstate);
  bindLuaSymbol(api.module, "luaL_openlibs", api.openlibs);
  bindLuaSymbol(api.module, "lua_close", api.close);
  bindLuaSymbol(api.module, "luaL_loadfilex", api.loadfilex);
  bindLuaSymbol(api.module, "luaL_loadbufferx", api.loadbufferx);
  bindLuaSymbol(api.module, "lua_pcallk", api.pcallk);
  bindLuaSymbol(api.module, "lua_gettop", api.gettop);
  bindLuaSymbol(api.module, "lua_checkstack", api.checkstack);
  bindLuaSymbol(api.module, "lua_settop", api.settop);
  bindLuaSymbol(api.module, "lua_type", api.type);
  bindLuaSymbol(api.module, "lua_tolstring", api.tolstring);
  bindLuaSymbol(api.module, "lua_tonumberx", api.tonumberx);
  bindLuaSymbol(api.module, "lua_tointegerx", api.tointegerx);
  bindLuaSymbol(api.module, "lua_toboolean", api.toboolean);
  bindLuaSymbol(api.module, "lua_touserdata", api.touserdata);
  bindLuaSymbol(api.module, "lua_pushnil", api.pushnil);
  bindLuaSymbol(api.module, "lua_pushboolean", api.pushboolean);
  bindLuaSymbol(api.module, "lua_pushnumber", api.pushnumber);
  bindLuaSymbol(api.module, "lua_pushinteger", api.pushinteger);
  bindLuaSymbol(api.module, "lua_pushstring", api.pushstring);
  bindLuaSymbol(api.module, "lua_pushlstring", api.pushlstring);
  bindLuaSymbol(api.module, "lua_pushlightuserdata", api.pushlightuserdata);
  bindLuaSymbol(api.module, "lua_pushvalue", api.pushvalue);
  bindLuaSymbol(api.module, "lua_pushcclosure", api.pushcclosure);
  bindLuaSymbol(api.module, "lua_createtable", api.createtable);
  bindLuaSymbol(api.module, "lua_setfield", api.setfield);
  bindLuaSymbol(api.module, "lua_getfield", api.getfield);
  bindLuaSymbol(api.module, "lua_setglobal", api.setglobal);
  bindLuaSymbol(api.module, "luaL_ref", api.ref);
  bindLuaSymbol(api.module, "luaL_unref", api.unref);
  bindLuaSymbol(api.module, "lua_rawgeti", api.rawgeti);
  bindLuaSymbol(api.module, "lua_rawseti", api.rawseti);
  bindLuaSymbol(api.module, "lua_next", api.next);
  bindLuaSymbol(api.module, "lua_settable", api.settable);
  bindLuaSymbol(api.module, "lua_rawlen", api.rawlen);
  if(!api.ready()) {
    runtimeLog("", "error", "lua54.dll is missing required Lua C API symbols");
  }
#else
  runtimeLog("", "error", "Lua mod loading is not wired for this platform yet");
#endif
  return api;
}
void runtimeLog(const std::string& modId, const char* level, const std::string& message) {
  std::cout << "[lua-mod";
  if(!modId.empty()) {
    std::cout << ':' << modId;
  }
  if(level != nullptr && *level != '\0') {
    std::cout << ':' << level;
  }
  std::cout << "] " << message << std::endl;
}
void pop(lua_State* state, int count) {
  LuaApi& api = luaApi();
  api.settop(state, api.gettop(state) - count);
}
std::string luaString(lua_State* state, int index, std::string fallback) {
  LuaApi& api = luaApi();
  if(!api.ready() || api.type(state, index) != kLuaTString) {
    return fallback;
  }
  const char* value = api.tolstring(state, index, nullptr);
  return value != nullptr ? value : fallback;
}
bool luaBoolField(lua_State* state, int tableIndex, const char* key, bool fallback) {
  LuaApi& api = luaApi();
  api.getfield(state, tableIndex, key);
  bool value = fallback;
  if(api.type(state, -1) == kLuaTBoolean) {
    value = api.toboolean(state, -1) != 0;
  }
  api.settop(state, -2);
  return value;
}
float luaFloatField(lua_State* state, int tableIndex, const char* key, float fallback) {
  LuaApi& api = luaApi();
  api.getfield(state, tableIndex, key);
  float value = fallback;
  int isNumber = 0;
  const double number = api.tonumberx(state, -1, &isNumber);
  if(isNumber != 0) {
    value = static_cast<float>(number);
  }
  api.settop(state, -2);
  return value;
}
float luaFloatAt(lua_State* state, int tableIndex, int index, float fallback) {
  LuaApi& api = luaApi();
  api.rawgeti(state, tableIndex, index);
  int isNumber = 0;
  const double number = api.tonumberx(state, -1, &isNumber);
  api.settop(state, -2);
  return isNumber != 0 ? static_cast<float>(number) : fallback;
}
int luaIntField(lua_State* state, int tableIndex, const char* key, int fallback) {
  LuaApi& api = luaApi();
  api.getfield(state, tableIndex, key);
  int value = fallback;
  int isNumber = 0;
  const long long number = api.tointegerx(state, -1, &isNumber);
  if(isNumber != 0) {
    value = static_cast<int>(number);
  }
  api.settop(state, -2);
  return value;
}
std::string luaStringField(lua_State* state, int tableIndex, const char* key, std::string fallback) {
  LuaApi& api = luaApi();
  api.getfield(state, tableIndex, key);
  std::string value = std::move(fallback);
  if(api.type(state, -1) == kLuaTString) {
    const char* text = api.tolstring(state, -1, nullptr);
    if(text != nullptr) {
      value = text;
    }
  }
  api.settop(state, -2);
  return value;
}
double luaDoubleField(lua_State* state, int tableIndex, const char* key, double fallback) {
  LuaApi& api = luaApi();
  api.getfield(state, tableIndex, key);
  int isNumber = 0;
  const double value = api.tonumberx(state, -1, &isNumber);
  api.settop(state, -2);
  return isNumber != 0 ? value : fallback;
}
int luaIntArg(lua_State* state, int index, int fallback) {
  int isNumber = 0;
  const double value = luaApi().tonumberx(state, index, &isNumber);
  return isNumber == 0 ? fallback : static_cast<int>(value);
}
float luaFloatArg(lua_State* state, int index, float fallback) {
  int isNumber = 0;
  const double value = luaApi().tonumberx(state, index, &isNumber);
  return isNumber == 0 ? fallback : static_cast<float>(value);
}
double luaDoubleArg(lua_State* state, int index, double fallback) {
  int isNumber = 0;
  const double value = luaApi().tonumberx(state, index, &isNumber);
  return isNumber == 0 ? fallback : value;
}
void readField(lua_State* state, const char* key, bool& value) {
  value = luaBoolField(state, -1, key, value);
}
void readField(lua_State* state, const char* key, int& value) {
  value = luaIntField(state, -1, key, value);
}
void readField(lua_State* state, const char* key, float& value) {
  value = luaFloatField(state, -1, key, value);
}
void readField(lua_State* state, const char* key, double& value) {
  value = luaDoubleField(state, -1, key, value);
}
void setField(lua_State* state, const char* key, bool value) {
  LuaApi& api = luaApi();
  api.pushboolean(state, value ? 1 : 0);
  api.setfield(state, -2, key);
}
void setField(lua_State* state, const char* key, int value) {
  LuaApi& api = luaApi();
  api.pushinteger(state, value);
  api.setfield(state, -2, key);
}
void setField(lua_State* state, const char* key, std::int64_t value) {
  LuaApi& api = luaApi();
  api.pushinteger(state, static_cast<long long>(value));
  api.setfield(state, -2, key);
}
void setField(lua_State* state, const char* key, float value) {
  LuaApi& api = luaApi();
  api.pushnumber(state, static_cast<double>(value));
  api.setfield(state, -2, key);
}
void setField(lua_State* state, const char* key, double value) {
  LuaApi& api = luaApi();
  api.pushnumber(state, value);
  api.setfield(state, -2, key);
}
void setField(lua_State* state, const char* key, const char* value) {
  LuaApi& api = luaApi();
  api.pushstring(state, value != nullptr ? value : "");
  api.setfield(state, -2, key);
}
void setField(lua_State* state, const char* key, const std::string& value) {
  LuaApi& api = luaApi();
  api.pushstring(state, value.c_str());
  api.setfield(state, -2, key);
}
void bindFunctions(lua_State* state, std::initializer_list<LuaBinding> bindings) {
  LuaApi& api = luaApi();
  for(const LuaBinding& binding : bindings) {
    api.pushcclosure(state, binding.function, 0);
    api.setfield(state, -2, binding.name);
  }
}
void pushFunctionTable(lua_State* state, std::initializer_list<LuaBinding> bindings) {
  luaApi().createtable(state, 0, static_cast<int>(bindings.size()));
  bindFunctions(state, bindings);
}
void bindModFunction(lua_State* state, void* modContext, const char* name, LuaCFunction function) {
  LuaApi& api = luaApi();
  api.pushlightuserdata(state, modContext);
  api.pushcclosure(state, function, 1);
  api.setfield(state, -2, name);
}
} // namespace net::minecraft::mod::lua
