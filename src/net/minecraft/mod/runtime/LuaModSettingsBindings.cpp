#include "net/minecraft/mod/runtime/LuaModSettingsBindings.hpp"
#include <algorithm>
#include <iterator>
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
  } else if(kind == "options" || kind == "cycle" || kind == "enum") {
   def.kind = net::minecraft::mod::ModSettingDef::Options;
   api.getfield(state, entry, "options");
   if(api.type(state, -1) == kLuaTTable) {
    const int optsTable = api.gettop(state);
    int optIdx = 1;
    while(true) {
     api.rawgeti(state, optsTable, optIdx);
     if(api.type(state, -1) == kLuaTNil) {
      api.settop(state, optsTable - 1);
      break;
     }
     if(api.type(state, -1) == kLuaTString) {
      def.options.push_back(luaString(state, -1, ""));
     }
     api.settop(state, optsTable);
     optIdx++;
    }
   }
   api.settop(state, entry);
   api.getfield(state, entry, "default");
   if(api.type(state, -1) == kLuaTString) {
    std::string defStr = luaString(state, -1, "");
    auto it = std::find(def.options.begin(), def.options.end(), defStr);
    if(it != def.options.end()) {
     def.optionDefault = static_cast<int>(std::distance(def.options.begin(), it));
    }
   } else if(api.type(state, -1) == kLuaTNumber) {
    def.optionDefault = static_cast<int>(api.tointegerx(state, -1, nullptr));
   }
   api.settop(state, entry);
   def.optionCurrent = def.optionDefault;
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
 const std::string fullKey = luaString(state, 1, "");
 if(fullKey.empty()) {
  api.pushnil(state);
  return 1;
 }
 auto dot = fullKey.find('.');
 if(dot == std::string::npos) {
  api.pushnil(state);
  return 1;
 }
 std::string modId = fullKey.substr(0, dot);
 std::string key = fullKey.substr(dot + 1);
 const auto* def = net::minecraft::mod::ModSettingsRegistry::instance().findSetting(modId, key);
 if(def == nullptr) {
  api.pushnil(state);
  return 1;
 }
 if(def->kind == net::minecraft::mod::ModSettingDef::Toggle) {
  api.pushboolean(state, def->boolCurrent ? 1 : 0);
 } else if(def->kind == net::minecraft::mod::ModSettingDef::Options) {
  if(def->optionCurrent >= 0 && def->optionCurrent < static_cast<int>(def->options.size())) {
   api.pushstring(state, def->options[static_cast<std::size_t>(def->optionCurrent)].c_str());
  } else {
   api.pushinteger(state, def->optionCurrent);
  }
 } else {
  api.pushnumber(state, static_cast<double>(def->floatCurrent));
 }
 return 1;
}
int luaSettingsSet(lua_State* state) {
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr) {
  return 0;
 }
 LuaApi& api = luaApi();
 const std::string fullKey = luaString(state, 1, "");
 if(fullKey.empty() || api.gettop(state) < 2) {
  return 0;
 }
 auto dot = fullKey.find('.');
 if(dot == std::string::npos) {
  return 0;
 }
 std::string modId = fullKey.substr(0, dot);
 std::string key = fullKey.substr(dot + 1);
 auto* def = net::minecraft::mod::ModSettingsRegistry::instance().findSetting(modId, key);
 if(def == nullptr) {
  return 0;
 }
 if(def->kind == net::minecraft::mod::ModSettingDef::Toggle) {
  def->boolCurrent = api.toboolean(state, 2) ? true : false;
 } else if(def->kind == net::minecraft::mod::ModSettingDef::Options) {
  if(api.type(state, 2) == kLuaTString) {
   const std::string val = luaString(state, 2, "");
   auto it = std::find(def->options.begin(), def->options.end(), val);
   if(it != def->options.end()) {
    def->optionCurrent = static_cast<int>(std::distance(def->options.begin(), it));
   }
  } else if(api.type(state, 2) == kLuaTNumber) {
   const int idx = static_cast<int>(api.tointegerx(state, 2, nullptr));
   if(idx >= 0 && idx < static_cast<int>(def->options.size())) {
    def->optionCurrent = idx;
   }
  }
 } else {
  def->floatCurrent = static_cast<float>(api.tonumberx(state, 2, nullptr));
 }
 net::minecraft::mod::ModSettingsRegistry::instance().save();
 return 0;
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
int luaKeybindsConsume(lua_State* state) {
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 LuaApi& api = luaApi();
 const std::string id = luaString(state, 1, "");
 if(id.empty()) {
  api.pushboolean(state, 0);
  return 1;
 }
 std::string fullId = id;
 if(mod != nullptr && id.find('.') == std::string::npos) {
  fullId = mod->modId + "." + id;
 }
 bool consumed = net::minecraft::mod::ModSettingsRegistry::instance().consumeKeybind(fullId);
 if(!consumed) {
  consumed = net::minecraft::mod::ModSettingsRegistry::instance().consumeKeybind(id);
 }
 api.pushboolean(state, consumed ? 1 : 0);
 return 1;
}
#endif
} // namespace
void installModSettingsApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 LuaApi& api = luaApi();
 api.createtable(state, 0, 3);
 bindModFunction(state, &mod, "register", luaSettingsRegister);
 bindModFunction(state, &mod, "get", luaSettingsGet);
 bindModFunction(state, &mod, "set", luaSettingsSet);
 api.setfield(state, -2, "settings");
 api.createtable(state, 0, 3);
 bindModFunction(state, &mod, "register", luaKeybindsRegister);
 bindModFunction(state, &mod, "get_code", luaKeybindsGetCode);
 bindModFunction(state, &mod, "consume", luaKeybindsConsume);
 api.setfield(state, -2, "keybinds");
#else
 (void)state;
 (void)mod;
#endif
}
} // namespace net::minecraft::mod::runtime
