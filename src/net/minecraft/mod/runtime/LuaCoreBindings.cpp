#include "net/minecraft/mod/runtime/LuaCoreBindings.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaNbtCodec.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/LuaEventSubscribers.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/KeyBinding.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#endif
#include <chrono>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include "net/minecraft/nbt/Compression.hpp"
#include "net/minecraft/nbt/Nbt.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
[[nodiscard]] std::optional<LifecyclePhase> lifecyclePhaseFromName(std::string_view rawName) {
  static const std::unordered_map<std::string, LifecyclePhase> kPhases = {
      {"init", LifecyclePhase::Init},
      {"post_init", LifecyclePhase::PostInit},
      {"ready", LifecyclePhase::Ready},
  };
  const auto it = kPhases.find(toLowerCopy(std::string(rawName)));
  if(it == kPhases.end()) {
    return std::nullopt;
  }
  return it->second;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
const client::option::OptionSpec* findOptionSpec(std::string_view rawKey) {
  if(rawKey.empty()) {
    return nullptr;
  }
  const std::optional<const client::option::OptionSpec*> spec = client::option::OptionRegistry::byKey(rawKey);
  return spec ? *spec : nullptr;
}
void pushSavedOptionValue(lua_State* state,
                          const client::option::GameOptions& options,
                          const client::option::OptionSpec& spec) {
  LuaApi& api = luaApi();
  if(spec.save == nullptr) {
    api.pushnil(state);
    return;
  }
  std::ostringstream out;
  spec.save(options, out);
  const std::string text = out.str();
  if(text == "true" || text == "false") {
    api.pushboolean(state, text == "true" ? 1 : 0);
    return;
  }
  try {
    std::size_t pos = 0;
    const int intValue = std::stoi(text, &pos);
    if(pos == text.size()) {
      api.pushinteger(state, static_cast<long long>(intValue));
      return;
    }
  } catch(...) {
  }
  try {
    const double floatValue = std::stod(text);
    api.pushnumber(state, floatValue);
    return;
  } catch(...) {
  }
  api.pushstring(state, text.c_str());
}
void pushOptionSpecValue(lua_State* state,
                         const client::option::GameOptions& options,
                         const client::option::OptionSpec& spec) {
  LuaApi& api = luaApi();
  switch(spec.kind) {
  case client::option::OptionSpec::Kind::Toggle:
    if(spec.getBool != nullptr) {
      api.pushboolean(state, spec.getBool(options) ? 1 : 0);
    } else {
      pushSavedOptionValue(state, options, spec);
    }
    break;
  case client::option::OptionSpec::Kind::Slider:
    if(spec.getFloat != nullptr) {
      api.pushnumber(state, static_cast<double>(spec.getFloat(options)));
    } else {
      pushSavedOptionValue(state, options, spec);
    }
    break;
  case client::option::OptionSpec::Kind::Cycle:
  case client::option::OptionSpec::Kind::Hidden:
  default:
    pushSavedOptionValue(state, options, spec);
    break;
  }
}
bool tryPushKeybindOption(lua_State* state, const client::option::GameOptions& options, std::string_view rawKey) {
  const std::string key(rawKey);
  const std::string lower = toLowerCopy(key);
  LuaApi& api = luaApi();
  for(int i = 0; i < client::option::GameOptions::kKeybindCount; ++i) {
    const client::option::KeyBinding* binding = options.allKeys[i];
    if(binding == nullptr) {
      continue;
    }
    const std::string translation = binding->translationKey;
    const std::string shortName = translation.rfind("key.", 0) == 0 ? translation.substr(4) : translation;
    if(lower == toLowerCopy(translation) || lower == toLowerCopy(shortName)) {
      api.pushinteger(state, static_cast<long long>(binding->code));
      return true;
    }
  }
  return false;
}
bool tryPushExtraOption(lua_State* state, const client::option::GameOptions& options, std::string_view rawKey) {
  const std::string lower = toLowerCopy(std::string(rawKey));
  LuaApi& api = luaApi();
  if(lower == "skin") {
    api.pushstring(state, options.skin.c_str());
    return true;
  }
  if(lower == "lastserver") {
    api.pushstring(state, options.lastServer.c_str());
    return true;
  }
  return tryPushKeybindOption(state, options, rawKey);
}
int luaOptionsGet(lua_State* state) {
  LuaApi& api = luaApi();
  if(client::Minecraft::INSTANCE == nullptr || api.type(state, 1) != kLuaTString) {
    api.pushnil(state);
    return 1;
  }
  const std::string_view rawKey = luaString(state, 1, "");
  const client::option::GameOptions& options = client::Minecraft::INSTANCE->options;
  if(const client::option::OptionSpec* spec = findOptionSpec(rawKey)) {
    pushOptionSpecValue(state, options, *spec);
    return 1;
  }
  if(tryPushExtraOption(state, options, rawKey)) {
    return 1;
  }
  api.pushnil(state);
  return 1;
}
int luaOptionsCycle(lua_State* state) {
  LuaApi& api = luaApi();
  if(client::Minecraft::INSTANCE == nullptr || api.type(state, 1) != kLuaTString) {
    return 0;
  }
  const std::string_view rawKey = luaString(state, 1, "");
  int isNumber = 0;
  const int delta = static_cast<int>(api.tointegerx(state, 2, &isNumber));
  client::Minecraft::INSTANCE->options.cycle(rawKey, isNumber != 0 ? delta : 1);
  return 0;
}
int luaOptionsKeys(lua_State* state) {
  LuaApi& api = luaApi();
  const std::span<const client::option::OptionSpec> specs = client::option::OptionRegistry::all();
  api.createtable(state, static_cast<int>(specs.size()), 0);
  for(std::size_t i = 0; i < specs.size(); ++i) {
    api.pushstring(state, std::string(specs[i].persistKey).c_str());
    api.rawseti(state, -2, static_cast<long long>(i + 1));
  }
  return 1;
}
#endif
int luaLog(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  const int top = api.gettop(state);
  const std::string level = top >= 2 ? luaString(state, 1, "info") : "info";
  const std::string message = top >= 2 ? luaString(state, 2, "") : luaString(state, 1, "");
  runtimeLog(mod != nullptr ? mod->modId : "", level.c_str(), message);
  return 0;
}
int luaTimeUtcMillis(lua_State* state) {
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  luaApi().pushnumber(state, static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count()));
  return 1;
}
int luaIsKeyDown(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 1) {
    api.pushboolean(state, 0);
    return 1;
  }
  int isNumber = 0;
  const int key = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  if(isNumber == 0) {
    api.pushboolean(state, 0);
    return 1;
  }
#ifdef MINECRAFT_NATIVE_EXPORTS
  api.pushboolean(state, client::input::InputSystem::instance().isKeyDown(key) ? 1 : 0);
#else
  (void)key;
  api.pushboolean(state, 0);
#endif
  return 1;
}
int luaIsMouseDown(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 1) {
    api.pushboolean(state, 0);
    return 1;
  }
  int isNumber = 0;
  const int button = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  if(isNumber == 0) {
    api.pushboolean(state, 0);
    return 1;
  }
#ifdef MINECRAFT_NATIVE_EXPORTS
  api.pushboolean(state, client::input::InputSystem::instance().isMouseButtonDown(button) ? 1 : 0);
#else
  (void)button;
  api.pushboolean(state, 0);
#endif
  return 1;
}
int defaultKeyCodeForName(const std::string& name) {
  static const std::unordered_map<std::string, int> kNamedCodes = {
      {"escape", 1},
      {"1", 2},
      {"2", 3},
      {"3", 4},
      {"4", 5},
      {"5", 6},
      {"6", 7},
      {"7", 8},
      {"8", 9},
      {"9", 10},
      {"0", 11},
      {"q", 16},
      {"w", 17},
      {"e", 18},
      {"r", 19},
      {"t", 20},
      {"y", 21},
      {"u", 22},
      {"i", 23},
      {"o", 24},
      {"p", 25},
      {"enter", 28},
      {"a", 30},
      {"s", 31},
      {"d", 32},
      {"f", 33},
      {"g", 34},
      {"h", 35},
      {"j", 36},
      {"k", 37},
      {"l", 38},
      {"z", 44},
      {"x", 45},
      {"c", 46},
      {"v", 47},
      {"b", 48},
      {"n", 49},
      {"m", 50},
      {"space", 57},
      {"up", 200},
      {"left_arrow", 203},
      {"right_arrow", 205},
      {"down", 208},
  };
  if(const auto it = kNamedCodes.find(name); it != kNamedCodes.end()) {
    return it->second;
  }
  return 0;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
int keyCodeFromBindingName(const std::string& name, const client::option::GameOptions& options) {
  if(name == "forward") {
    return options.forwardKey.code;
  }
  if(name == "left") {
    return options.leftKey.code;
  }
  if(name == "back") {
    return options.backKey.code;
  }
  if(name == "right") {
    return options.rightKey.code;
  }
  if(name == "jump") {
    return options.jumpKey.code;
  }
  if(name == "sneak") {
    return options.sneakKey.code;
  }
  if(name == "drop") {
    return options.dropKey.code;
  }
  if(name == "inventory") {
    return options.inventoryKey.code;
  }
  if(name == "chat") {
    return options.chatKey.code;
  }
  if(name == "fog") {
    return options.fogKey.code;
  }
  return 0;
}
#endif
int luaKeyCode(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 1) {
    api.pushinteger(state, 0);
    return 1;
  }
  if(api.type(state, 1) == kLuaTNumber) {
    int isNumber = 0;
    const int key = static_cast<int>(api.tointegerx(state, 1, &isNumber));
    api.pushinteger(state, isNumber != 0 ? key : 0);
    return 1;
  }
  std::string name = luaString(state, 1, "");
#ifdef MINECRAFT_NATIVE_EXPORTS
  if(client::Minecraft* minecraft = client::Minecraft::INSTANCE; minecraft != nullptr) {
    if(const int bound = keyCodeFromBindingName(name, minecraft->options); bound != 0) {
      api.pushinteger(state, bound);
      return 1;
    }
  }
#endif
  api.pushinteger(state, defaultKeyCodeForName(name));
  return 1;
}
int luaAssetPath(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1) {
    api.pushnil(state);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  const std::filesystem::path path = host().assetPath(mod->modId, relativePath);
  if(path.empty()) {
    api.pushnil(state);
    return 1;
  }
  const std::string text = path.string();
  api.pushstring(state, text.c_str());
  return 1;
}
int luaReadAsset(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1) {
    api.pushnil(state);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  const std::filesystem::path path = host().assetPath(mod->modId, relativePath);
  if(path.empty() || !std::filesystem::is_regular_file(path)) {
    api.pushnil(state);
    return 1;
  }
  std::ifstream input(path, std::ios::binary);
  if(!input.is_open()) {
    api.pushnil(state);
    return 1;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string content = buffer.str();
  api.pushstring(state, content.c_str());
  return 1;
}
int luaReadAssetBytes(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTString) {
    api.pushnil(state);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  bool gzip = false;
  if(api.gettop(state) >= 2 && api.type(state, 2) == kLuaTTable) {
    gzip = luaBoolField(state, 2, "gzip", false);
  }
  const std::filesystem::path path = host().assetPath(mod->modId, relativePath);
  if(path.empty() || !std::filesystem::is_regular_file(path)) {
    api.pushnil(state);
    return 1;
  }
  std::ifstream input(path, std::ios::binary);
  if(!input.is_open()) {
    api.pushnil(state);
    return 1;
  }
  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  if(bytes.empty()) {
    api.pushnil(state);
    return 1;
  }
  if(gzip) {
    try {
      bytes = gzipDecompress(bytes);
    } catch(const std::exception&) {
      api.pushnil(state);
      return 1;
    }
  }
  if(bytes.empty()) {
    api.pushnil(state);
    return 1;
  }
  api.pushlstring(state, reinterpret_cast<const char*>(bytes.data()), bytes.size());
  return 1;
}
int luaReadNbtAsset(lua_State* state) {
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  const std::string relativePath = normalizeRelativePath(luaString(state, 1, ""));
  const std::filesystem::path path =
      mod != nullptr ? host().assetPath(mod->modId, relativePath) : std::filesystem::path{};
  if(path.empty() || !std::filesystem::is_regular_file(path) ||
     std::filesystem::file_size(path) > kMaxModEntryBytes) {
    luaApi().pushnil(state);
    luaApi().pushstring(state, "NBT asset not found or too large");
    return 2;
  }
  try {
    const std::vector<std::uint8_t> bytes = readFileBytes(path);
    const bool gzip = bytes.size() >= 2 && bytes[0] == 0x1f && bytes[1] == 0x8b;
    const Nbt root = gzip ? Nbt::readCompressed(bytes) : Nbt::read(bytes);
    pushNbtValue(state, root);
    luaApi().pushnil(state);
  } catch(const std::exception& error) {
    luaApi().pushnil(state);
    luaApi().pushstring(state, error.what());
  }
  return 2;
}
int luaReadGameFile(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTString) {
    api.pushnil(state);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  if(relativePath.empty() || !isSafeRelativePath(relativePath)) {
    api.pushnil(state);
    return 1;
  }
  const std::filesystem::path path = host().runDirectory() / "config" / "mods" / sanitizeName(mod->modId) /
                                     std::filesystem::path(normalizeRelativePath(relativePath));
  if(!std::filesystem::is_regular_file(path)) {
    api.pushnil(state);
    return 1;
  }
  const std::string content = readFileText(path);
  api.pushlstring(state, content.data(), content.size());
  return 1;
}
int luaWriteGameFile(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 2 || api.type(state, 1) != kLuaTString) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  const std::string content = luaString(state, 2, "");
  if(relativePath.empty() || !isSafeRelativePath(relativePath)) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::filesystem::path path = host().runDirectory() / "config" / "mods" / sanitizeName(mod->modId) /
                                     std::filesystem::path(normalizeRelativePath(relativePath));
  api.pushboolean(state, writeFileText(path, content) ? 1 : 0);
  return 1;
}
int luaOn(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  const int top = api.gettop(state);
  if(mod == nullptr || top < 2 || api.type(state, 1) != kLuaTString || api.type(state, 2) != kLuaTFunction) {
    runtimeLog(mod != nullptr ? mod->modId : "",
               "error",
               "minecraft._subscribe expects (event_name, function, priority?)");
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string eventName = toLowerCopy(luaString(state, 1, ""));
  if(!isSupportedLuaEvent(eventName)) {
    runtimeLog(mod->modId, "error", "unknown Lua event: " + eventName);
    api.pushboolean(state, 0);
    return 1;
  }
  int priority = 0;
  if(top >= 3) {
    int isNumber = 0;
    const long long value = api.tointegerx(state, 3, &isNumber);
    if(isNumber != 0) {
      priority = static_cast<int>(value);
    }
  }
  api.pushvalue(state, 2);
  const int ref = api.ref(state, kLuaRegistryIndex);
  if(ref == kLuaNoRef) {
    runtimeLog(mod->modId, "error", "failed to retain Lua callback");
    api.pushboolean(state, 0);
    return 1;
  }
  mod->callbacks.push_back({eventName, ref, priority});
  api.pushboolean(state, 1);
  return 1;
}
int luaAtPhase(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 3 || api.type(state, 1) != kLuaTString ||
     api.type(state, 3) != kLuaTFunction) {
    runtimeLog(
        mod != nullptr ? mod->modId : "", "error", "minecraft.at_phase expects (phase_name, order, function)");
    return 0;
  }
  const std::optional<LifecyclePhase> phase = lifecyclePhaseFromName(luaString(state, 1, ""));
  if(!phase.has_value()) {
    runtimeLog(mod->modId, "error", "unknown lifecycle phase: " + luaString(state, 1, ""));
    return 0;
  }
  int order = 0;
  int isNumber = 0;
  const long long orderValue = api.tointegerx(state, 2, &isNumber);
  if(isNumber != 0) {
    order = static_cast<int>(orderValue);
  }
  api.pushvalue(state, 3);
  const int ref = api.ref(state, kLuaRegistryIndex);
  if(ref == kLuaNoRef) {
    runtimeLog(mod->modId, "error", "failed to retain Lua lifecycle callback");
    return 0;
  }
  const LifecyclePhase targetPhase = *phase;
  const std::string modId = mod->modId;
  hooks().subscribe<LifecycleEvent>(order, [modId, ref, targetPhase](LifecycleEvent& event) {
    if(event.current != targetPhase) {
      return;
    }
    for(const std::shared_ptr<ModHost::LoadedLuaMod>& loaded : host().loadedMods()) {
      if(loaded == nullptr || !loaded->active || loaded->modId != modId) {
        continue;
      }
      callLuaEvent(
          loaded,
          ref,
          [&event](lua_State* luaState) {
            setField(luaState, "previous", static_cast<int>(event.previous));
            setField(luaState, "current", static_cast<int>(event.current));
          },
          [](lua_State*) {});
      break;
    }
  });
  return 0;
}
} // namespace
int luaIsClient(lua_State* state) {
  luaApi().pushboolean(state, modContextIsClient() ? 1 : 0);
  return 1;
}
void installCoreApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
  LuaApi& api = luaApi();
  bindModFunction(state, &mod, "log", luaLog);
  bindModFunction(state, &mod, "is_client", luaIsClient);
  bindModFunction(state, &mod, "asset_path", luaAssetPath);
  bindModFunction(state, &mod, "read_asset", luaReadAsset);
  bindModFunction(state, &mod, "read_asset_bytes", luaReadAssetBytes);
  bindModFunction(state, &mod, "read_nbt_asset", luaReadNbtAsset);
  bindModFunction(state, &mod, "_read_storage", luaReadGameFile);
  bindModFunction(state, &mod, "_write_storage", luaWriteGameFile);
  bindModFunction(state, &mod, "_subscribe", luaOn);
  bindModFunction(state, &mod, "at_phase", luaAtPhase);
  bindFunctions(state,
                {
                    {"is_key_down", luaIsKeyDown},
                    {"is_mouse_down", luaIsMouseDown},
                    {"key_code", luaKeyCode},
                });
  pushFunctionTable(state, {{"utc_millis", luaTimeUtcMillis}});
  api.setfield(state, -2, "time");
#ifdef MINECRAFT_NATIVE_EXPORTS
  pushFunctionTable(state, {{"get", luaOptionsGet}, {"keys", luaOptionsKeys}, {"cycle", luaOptionsCycle}});
  api.setfield(state, -2, "options");
#endif
}
} // namespace net::minecraft::mod::runtime
