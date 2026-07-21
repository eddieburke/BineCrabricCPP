#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#endif
#include "net/minecraft/world/World.hpp"
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
LuaApi gLuaApi;
bool gLuaApiLoaded = false;
void loadLuaApi() {
 if(gLuaApiLoaded) {
  return;
 }
 gLuaApiLoaded = true;
 LuaApi& api = gLuaApi;
#ifdef _WIN32
 api.module = LoadLibraryW(L"lua54.dll");
 if(api.module == nullptr) {
  std::filesystem::path bundled =
      std::filesystem::path(MINECRAFT_NATIVE_SOURCE_DIR) / "toolchain" / "mingw64" / "bin" / "lua54.dll";
  api.module = LoadLibraryW(bundled.wstring().c_str());
 }
 if(api.module == nullptr) {
  runtimeLog("", "error", "lua54.dll not found; Lua mods cannot load");
  return;
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
 luaApi().settop(state, -count - 1);
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
// ── Mod execution context (thread-local) ─────────────────────────────
namespace {
thread_local World* gModContextWorld = nullptr;
#ifdef MINECRAFT_NATIVE_EXPORTS
thread_local bool gModContextIsClient = true;
#else
thread_local bool gModContextIsClient = false;
#endif
thread_local entity::player::PlayerEntity* gModContextPlayer = nullptr;
} // namespace
World* contextOrClientWorld() {
 if(gModContextWorld != nullptr) {
  return gModContextWorld;
 }
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(client::Minecraft::INSTANCE != nullptr) {
  return client::Minecraft::INSTANCE->world;
 }
#endif
 return nullptr;
}
ModContextScope::ModContextScope(World* world, entity::player::PlayerEntity* player)
    : previousWorld_(gModContextWorld),
      previousIsClient_(gModContextIsClient),
      previousPlayer_(gModContextPlayer) {
 gModContextWorld = world;
 gModContextIsClient = world != nullptr && world->isRemote();
 gModContextPlayer = player;
}
ModContextScope::~ModContextScope() {
 gModContextWorld = previousWorld_;
 gModContextIsClient = previousIsClient_;
 gModContextPlayer = previousPlayer_;
}
void setModContext(World* world, bool isClient, entity::player::PlayerEntity* player) {
 gModContextWorld = world;
 gModContextIsClient = isClient;
 gModContextPlayer = player;
}
void clearModContext() {
 gModContextWorld = nullptr;
#ifdef MINECRAFT_NATIVE_EXPORTS
 gModContextIsClient = true;
#else
 gModContextIsClient = false;
#endif
 gModContextPlayer = nullptr;
}
bool modContextIsClient() {
 return gModContextIsClient;
}
entity::player::PlayerEntity* activeModPlayer() {
 return gModContextPlayer;
}
// ── String / path sanitisation helpers ──────────────────────────────
std::string trimCopy(std::string value) {
 while(!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
  value.erase(value.begin());
 }
 while(!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
  value.pop_back();
 }
 return value;
}
std::string toLowerCopy(std::string value) {
 for(char& ch : value) {
  ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
 }
 return value;
}
std::string normalizeRelativePath(std::string_view value) {
 std::string normalized(value);
 std::replace(normalized.begin(), normalized.end(), '\\', '/');
 while(!normalized.empty() && normalized.front() == '/') {
  normalized.erase(normalized.begin());
 }
 return normalized;
}
bool isSafeRelativePath(std::string_view value) {
 const std::string normalized = normalizeRelativePath(value);
 if(normalized.empty() || normalized.find(':') != std::string::npos) {
  return false;
 }
 std::stringstream stream(normalized);
 std::string part;
 while(std::getline(stream, part, '/')) {
  if(part.empty() || part == ".") {
   continue;
  }
  if(part == "..") {
   return false;
  }
 }
 return true;
}
bool isSafeModId(std::string_view value) {
 return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
  return std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.';
 });
}
bool isDirectoryZipPath(std::string_view value) {
 const std::string normalized = normalizeRelativePath(value);
 return !normalized.empty() && normalized.back() == '/';
}
std::string sanitizeName(std::string_view value) {
 std::string out;
 out.reserve(value.size());
 for(char ch : value) {
  if(std::isalnum(static_cast<unsigned char>(ch))) {
   out.push_back(ch);
  } else {
   out.push_back('_');
  }
 }
 while(!out.empty() && out.back() == '_') {
  out.pop_back();
 }
 return out.empty() ? "mod" : out;
}
// ── File I/O helpers (mod-scoped) ───────────────────────────────────
std::vector<std::uint8_t> readFileBytes(const std::filesystem::path& path) {
 std::ifstream input(path, std::ios::binary);
 if(!input.is_open()) {
  return {};
 }
 input.seekg(0, std::ios::end);
 const std::streamsize size = input.tellg();
 input.seekg(0, std::ios::beg);
 if(size <= 0) {
  return {};
 }
 std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
 if(!input.read(reinterpret_cast<char*>(bytes.data()), size)) {
  return {};
 }
 return bytes;
}
std::string readFileText(const std::filesystem::path& path) {
 const std::vector<std::uint8_t> bytes = readFileBytes(path);
 return std::string(bytes.begin(), bytes.end());
}
bool writeFileBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
 std::filesystem::create_directories(path.parent_path());
 std::ofstream output(path, std::ios::binary | std::ios::trunc);
 if(!output.is_open()) {
  return false;
 }
 if(!bytes.empty()) {
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
 }
 return output.good();
}
bool writeFileText(const std::filesystem::path& path, const std::string& text) {
 std::filesystem::create_directories(path.parent_path());
 std::ofstream output(path, std::ios::binary | std::ios::trunc);
 if(!output.is_open()) {
  return false;
 }
 output << text;
 return output.good();
}
} // namespace net::minecraft::mod::lua
