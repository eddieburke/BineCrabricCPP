#include "net/minecraft/mod/runtime/LuaModSettingsBindings.hpp"
#include "net/minecraft/mod/ModSettingsRegistry.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
#ifdef MINECRAFT_NATIVE_EXPORTS
int luaSettingsRegister(lua_State* state) {
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr) {
  return 0;
 }
 LuaApi& api = luaApi();
 if(api.gettop(state) < 2 || api.type(state, 2) != kLuaTTable) {
  runtimeLog(mod->modId, "warn", "settings.register(modId, settingsTable) requires a table as 2nd argument");
  return 0;
 }
 const std::string modId = mod->modId;
 const std::string modName = luaString(state, 1, modId.c_str());
 const int settingsTable = 2;
 int idx = 1;
 while(true) {
  api.rawgeti(state, settingsTable, idx);
  if(api.type(state, -1) == kLuaTNil) {
   api.settop(state, settingsTable - 1);
   break;
  }
  if(api.type(state, -1) != kLuaTTable) {
   api.settop(state, settingsTable);
   idx++;
   continue;
  }
  const int entry = api.gettop(state);
  net::minecraft::mod::ModSettingDef def;
  def.key = luaStringField(state, entry, "key", "");
  if(def.key.empty()) {
   api.settop(state, entry - 1);
   idx++;
   continue;
  }
  def.label = luaStringField(state, entry, "label", def.key);
  const std::string kind = luaStringField(state, entry, "kind", "slider");
  if(kind == "toggle") {
   def.kind = net::minecraft::mod::ModSettingDef::Toggle;
   def.boolDefault = luaBoolField(state, entry, "default", false);
   def.boolCurrent = def.boolDefault;
  } else {
   def.kind = net::minecraft::mod::ModSettingDef::Slider;
   def.sliderMin = luaFloatField(state, entry, "min", 0.0f);
   def.sliderMax = luaFloatField(state, entry, "max", 1.0f);
   def.sliderStep = luaFloatField(state, entry, "step", 0.0f);
   def.sliderDecimals = luaIntField(state, entry, "decimals", 2);
   def.sliderInteger = luaBoolField(state, entry, "integer", false);
   def.floatDefault = luaFloatField(state, entry, "default", 0.0f);
   def.floatCurrent = def.floatDefault;
  }
  net::minecraft::mod::ModSettingsRegistry::instance().registerSetting(modId, modName, std::move(def));
  api.settop(state, entry - 1);
  idx++;
 }
 return 0;
}
int luaSettingsGet(lua_State* state) {
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr) {
  return 0;
 }
 LuaApi& api = luaApi();
 const std::string key = luaString(state, 1, "");
 const auto* def = net::minecraft::mod::ModSettingsRegistry::instance().findSetting(mod->modId, key);
 if(def == nullptr) {
  api.pushnil(state);
  return 1;
 }
 if(def->kind == net::minecraft::mod::ModSettingDef::Toggle) {
  api.pushboolean(state, def->boolCurrent ? 1 : 0);
 } else {
  api.pushnumber(state, static_cast<double>(def->floatCurrent));
 }
 return 1;
}
int luaKeybindsRegister(lua_State* state) {
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr) {
  return 0;
 }
 LuaApi& api = luaApi();
 if(api.gettop(state) < 2 || api.type(state, 2) != kLuaTTable) {
  runtimeLog(mod->modId, "warn", "keybinds.register(name, {default, label}) requires a table as 2nd argument");
  return 0;
 }
 const std::string id = luaString(state, 1, "");
 if(id.empty()) {
  api.settop(state, 1);
  return 0;
 }
 const int table = 2;
 net::minecraft::mod::ModKeybindDef kb;
 kb.id = mod->modId + "." + id;
 kb.label = luaStringField(state, table, "label", id);
 kb.defaultKeyCode = luaIntField(state, table, "default", 0);
 kb.currentKeyCode = kb.defaultKeyCode;
 net::minecraft::mod::ModSettingsRegistry::instance().registerKeybind(mod->modId, std::move(kb));
 api.settop(state, 1);
 return 0;
}
int luaKeybindsGetCode(lua_State* state) {
 LuaApi& api = luaApi();
 const std::string id = luaString(state, 1, "");
 if(id.empty()) {
  api.pushinteger(state, 0);
  return 1;
 }
 const auto* kb = net::minecraft::mod::ModSettingsRegistry::instance().findKeybind(id);
 if(kb == nullptr) {
  api.pushinteger(state, 0);
 } else {
  api.pushinteger(state, static_cast<long long>(kb->currentKeyCode));
 }
 return 1;
}
#endif
} // namespace
void installModSettingsApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 LuaApi& api = luaApi();
 api.createtable(state, 0, 2);
 bindModFunction(state, &mod, "register", luaSettingsRegister);
 bindModFunction(state, &mod, "get", luaSettingsGet);
 api.setfield(state, -2, "settings");
 api.createtable(state, 0, 2);
 bindModFunction(state, &mod, "register", luaKeybindsRegister);
 bindModFunction(state, &mod, "get_code", luaKeybindsGetCode);
 api.setfield(state, -2, "keybinds");
#else
 (void)state;
 (void)mod;
#endif
}
} // namespace net::minecraft::mod::runtime
