#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
struct lua_State;
namespace net::minecraft::mod::lua {
constexpr int kLuaOk = 0;
constexpr int kLuaNoRef = -2;
constexpr int kLuaRegistryIndex = -1001000;
constexpr int kLuaTNil = 0;
constexpr int kLuaTBoolean = 1;
constexpr int kLuaTNumber = 3;
constexpr int kLuaTString = 4;
constexpr int kLuaTFunction = 6;
constexpr int kLuaTTable = 5;
using LuaCFunction = int (*)(lua_State*);
using LuaKContext = std::intptr_t;
using LuaKFunction = int (*)(lua_State*, int, LuaKContext);
[[nodiscard]] int luaUpvalueIndex(int i);
struct LuaApi {
#ifdef _WIN32
  void* module = nullptr;
#endif
  lua_State* (*newstate)() = nullptr;
  void (*openlibs)(lua_State*) = nullptr;
  void (*close)(lua_State*) = nullptr;
  int (*loadfilex)(lua_State*, const char*, const char*) = nullptr;
  int (*loadbufferx)(lua_State*, const char*, std::size_t, const char*, const char*) = nullptr;
  int (*pcallk)(lua_State*, int, int, int, LuaKContext, LuaKFunction) = nullptr;
  int (*gettop)(lua_State*) = nullptr;
  void (*settop)(lua_State*, int) = nullptr;
  int (*type)(lua_State*, int) = nullptr;
  const char* (*tolstring)(lua_State*, int, std::size_t*) = nullptr;
  double (*tonumberx)(lua_State*, int, int*) = nullptr;
  long long (*tointegerx)(lua_State*, int, int*) = nullptr;
  int (*toboolean)(lua_State*, int) = nullptr;
  void* (*touserdata)(lua_State*, int) = nullptr;
  void (*pushnil)(lua_State*) = nullptr;
  void (*pushboolean)(lua_State*, int) = nullptr;
  void (*pushnumber)(lua_State*, double) = nullptr;
  void (*pushinteger)(lua_State*, long long) = nullptr;
  const char* (*pushstring)(lua_State*, const char*) = nullptr;
  const char* (*pushlstring)(lua_State*, const char*, std::size_t) = nullptr;
  void (*pushlightuserdata)(lua_State*, void*) = nullptr;
  void (*pushvalue)(lua_State*, int) = nullptr;
  void (*pushcclosure)(lua_State*, LuaCFunction, int) = nullptr;
  void (*createtable)(lua_State*, int, int) = nullptr;
  void (*setfield)(lua_State*, int, const char*) = nullptr;
  int (*getfield)(lua_State*, int, const char*) = nullptr;
  void (*setglobal)(lua_State*, const char*) = nullptr;
  int (*ref)(lua_State*, int) = nullptr;
  void (*unref)(lua_State*, int, int) = nullptr;
  int (*rawgeti)(lua_State*, int, long long) = nullptr;
  void (*rawseti)(lua_State*, int, long long) = nullptr;
  int (*next)(lua_State*, int) = nullptr;
  void (*settable)(lua_State*, int) = nullptr;
  std::size_t (*rawlen)(lua_State*, int) = nullptr;
  [[nodiscard]] bool ready() const;
};
[[nodiscard]] LuaApi& luaApi();
void runtimeLog(const std::string& modId, const char* level, const std::string& message);
void pop(lua_State* state, int count);
[[nodiscard]] std::string luaString(lua_State* state, int index, std::string fallback = {});
// Positional number args without the tonumberx/isNumber boilerplate.
[[nodiscard]] int luaIntArg(lua_State* state, int index, int fallback = 0);
[[nodiscard]] float luaFloatArg(lua_State* state, int index, float fallback = 0.0f);
[[nodiscard]] double luaDoubleArg(lua_State* state, int index, double fallback = 0.0);
[[nodiscard]] bool luaBoolField(lua_State* state, int tableIndex, const char* key, bool fallback);
[[nodiscard]] float luaFloatField(lua_State* state, int tableIndex, const char* key, float fallback);
[[nodiscard]] float luaFloatAt(lua_State* state, int tableIndex, int index, float fallback);
[[nodiscard]] int luaIntField(lua_State* state, int tableIndex, const char* key, int fallback);
[[nodiscard]] std::string luaStringField(lua_State* state, int tableIndex, const char* key, std::string fallback = {});
[[nodiscard]] double luaDoubleField(lua_State* state, int tableIndex, const char* key, double fallback);
void setField(lua_State* state, const char* key, bool value);
void setField(lua_State* state, const char* key, int value);
void setField(lua_State* state, const char* key, std::int64_t value);
void setField(lua_State* state, const char* key, float value);
void setField(lua_State* state, const char* key, double value);
void setField(lua_State* state, const char* key, const char* value);
void setField(lua_State* state, const char* key, const std::string& value);
// In-place readback of a field from the table at -1 (fallback = current value).
void readField(lua_State* state, const char* key, bool& value);
void readField(lua_State* state, const char* key, int& value);
void readField(lua_State* state, const char* key, float& value);
void readField(lua_State* state, const char* key, double& value);
// Variadic key/value batches over setField/readField.
inline void setFields(lua_State*) {}
template <typename T, typename... Rest>
void setFields(lua_State* state, const char* key, const T& value, Rest&&... rest) {
  setField(state, key, value);
  setFields(state, static_cast<Rest&&>(rest)...);
}
inline void readFields(lua_State*) {}
template <typename T, typename... Rest>
void readFields(lua_State* state, const char* key, T& value, Rest&&... rest) {
  readField(state, key, value);
  readFields(state, static_cast<Rest&&>(rest)...);
}
struct LuaBinding {
  const char* name;
  LuaCFunction function;
};
void bindFunctions(lua_State* state, std::initializer_list<LuaBinding> bindings);
void pushFunctionTable(lua_State* state, std::initializer_list<LuaBinding> bindings);
void bindModFunction(lua_State* state, void* modContext, const char* name, LuaCFunction function);
} // namespace net::minecraft::mod::lua
