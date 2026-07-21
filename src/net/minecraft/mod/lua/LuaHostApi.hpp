#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
struct lua_State;
namespace net::minecraft {
class World;
namespace entity::player {
class PlayerEntity;
}
} // namespace net::minecraft
namespace net::minecraft::mod::lua {
constexpr int kLuaOk = 0;
constexpr int kLuaNoRef = -2;
constexpr int kLuaRegistryIndex = -1001000;
constexpr int kLuaTNil = 0;
constexpr int kLuaTBoolean = 1;
constexpr int kLuaTLightUserdata = 2;
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
 int (*checkstack)(lua_State*, int) = nullptr;
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
extern LuaApi gLuaApi;
extern bool gLuaApiLoaded;
void loadLuaApi();
[[nodiscard]] inline LuaApi& luaApi() {
 if(!gLuaApiLoaded) {
  loadLuaApi();
 }
 return gLuaApi;
}
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
inline void setField(lua_State* state, const char* key, bool value) {
 luaApi().pushboolean(state, value ? 1 : 0);
 luaApi().setfield(state, -2, key);
}
inline void setField(lua_State* state, const char* key, int value) {
 luaApi().pushinteger(state, value);
 luaApi().setfield(state, -2, key);
}
inline void setField(lua_State* state, const char* key, std::int64_t value) {
 luaApi().pushinteger(state, static_cast<long long>(value));
 luaApi().setfield(state, -2, key);
}
inline void setField(lua_State* state, const char* key, float value) {
 luaApi().pushnumber(state, static_cast<double>(value));
 luaApi().setfield(state, -2, key);
}
inline void setField(lua_State* state, const char* key, double value) {
 luaApi().pushnumber(state, value);
 luaApi().setfield(state, -2, key);
}
inline void setField(lua_State* state, const char* key, const char* value) {
 luaApi().pushstring(state, value != nullptr ? value : "");
 luaApi().setfield(state, -2, key);
}
inline void setField(lua_State* state, const char* key, const std::string& value) {
 luaApi().pushstring(state, value.c_str());
 luaApi().setfield(state, -2, key);
}
inline void setField(lua_State* state, const char* key, std::string_view value) {
 luaApi().pushlstring(state, value.data(), value.size());
 luaApi().setfield(state, -2, key);
}
// In-place readback of a field from the table at -1 (fallback = current value).
void readField(lua_State* state, const char* key, bool& value);
void readField(lua_State* state, const char* key, int& value);
void readField(lua_State* state, const char* key, float& value);
void readField(lua_State* state, const char* key, double& value);
// Variadic key/value batches over setField/readField.
inline void setFields(lua_State*) {
}
template <typename T, typename... Rest>
void setFields(lua_State* state, const char* key, const T& value, Rest&&... rest) {
 setField(state, key, value);
 setFields(state, static_cast<Rest&&>(rest)...);
}
inline void readFields(lua_State*) {
}
template <typename T, typename... Rest>
void readFields(lua_State* state, const char* key, T& value, Rest&&... rest) {
 readField(state, key, value);
 readFields(state, static_cast<Rest&&>(rest)...);
}
// ── Mod execution context (thread-local) ─────────────────────────────
inline constexpr std::uintmax_t kMaxModArchiveBytes = 256U * 1024U * 1024U;
inline constexpr std::uint64_t kMaxModEntryBytes = 64U * 1024U * 1024U;
inline constexpr std::uint64_t kMaxModExtractedBytes = 512U * 1024U * 1024U;
inline constexpr std::uint16_t kMaxModArchiveEntries = 4096;

[[nodiscard]] World* contextOrClientWorld();
void setModContext(World* world, bool isClient, entity::player::PlayerEntity* player = nullptr);
void clearModContext();
[[nodiscard]] bool modContextIsClient();
[[nodiscard]] entity::player::PlayerEntity* activeModPlayer();

struct ModContextScope {
  World* previousWorld_ = nullptr;
  bool previousIsClient_ = false;
  entity::player::PlayerEntity* previousPlayer_ = nullptr;
  ModContextScope(World* world, entity::player::PlayerEntity* player = nullptr);
  ~ModContextScope();
};

// ── String / path sanitisation helpers ──────────────────────────────
std::string trimCopy(std::string value);
std::string toLowerCopy(std::string value);
std::string normalizeRelativePath(std::string_view value);
bool isSafeRelativePath(std::string_view value);
bool isSafeModId(std::string_view value);
bool isDirectoryZipPath(std::string_view value);
std::string sanitizeName(std::string_view value);

// ── File I/O helpers (mod-scoped) ───────────────────────────────────
std::vector<std::uint8_t> readFileBytes(const std::filesystem::path& path);
std::string readFileText(const std::filesystem::path& path);
bool writeFileBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes);
bool writeFileText(const std::filesystem::path& path, const std::string& text);

struct LuaBinding {
 const char* name;
 LuaCFunction function;
};
void bindFunctions(lua_State* state, std::initializer_list<LuaBinding> bindings);
void pushFunctionTable(lua_State* state, std::initializer_list<LuaBinding> bindings);
void bindModFunction(lua_State* state, void* modContext, const char* name, LuaCFunction function);
} // namespace net::minecraft::mod::lua
