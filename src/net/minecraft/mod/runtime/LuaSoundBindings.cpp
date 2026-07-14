#include "net/minecraft/mod/runtime/LuaSoundBindings.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/platform/audio/AudioEngine.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
float readFloatArg(lua_State* state, int index, float fallback) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < index) {
    return fallback;
  }
  int isNumber = 0;
  const double value = api.tonumberx(state, index, &isNumber);
  return isNumber != 0 ? static_cast<float>(value) : fallback;
}
std::string readStringArg(lua_State* state, int index, std::string fallback = {}) {
  LuaApi& api = luaApi();
  return api.gettop(state) >= index ? luaString(state, index, std::move(fallback)) : std::move(fallback);
}
client::Minecraft* clientInstance() {
  return client::Minecraft::INSTANCE;
}
bool registerSoundKind(client::platform::audio::AudioEngine& audio,
                       const std::string& id,
                       const std::filesystem::path& path,
                       std::string kind) {
  kind = toLowerCopy(std::move(kind));
  if(kind.empty() || kind == "effect") {
    audio.registerEffect(id, path);
    return true;
  }
  if(kind == "streaming") {
    audio.registerStreaming(id, path);
    return true;
  }
  if(kind == "music") {
    audio.registerMusic(id, path);
    return true;
  }
  return false;
}
} // namespace
#ifdef MINECRAFT_NATIVE_EXPORTS
int luaRegisterSound(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 2 || api.type(state, 1) != kLuaTString ||
     api.type(state, 2) != kLuaTString) {
    api.pushboolean(state, 0);
    api.pushstring(state, "minecraft.sound.register expects (id, path, kind?)");
    return 2;
  }
  const std::string id = luaString(state, 1, "");
  const std::string path = luaString(state, 2, "");
  const std::string kind = readStringArg(state, 3, "effect");
  if(id.empty() || path.empty()) {
    api.pushboolean(state, 0);
    api.pushstring(state, "sound id and path are required");
    return 2;
  }
  const std::optional<std::filesystem::path> resolved = host().resolveResourcePath(path);
  if(!resolved.has_value()) {
    api.pushboolean(state, 0);
    api.pushstring(state, ("missing sound file: " + path).c_str());
    return 2;
  }
  client::Minecraft* client = clientInstance();
  if(client == nullptr || !registerSoundKind(client->audio, id, *resolved, kind)) {
    api.pushboolean(state, 0);
    api.pushstring(state, "unknown sound kind");
    return 2;
  }
  api.pushboolean(state, 1);
  return 1;
}
int luaPlaySound(lua_State* state) {
  LuaApi& api = luaApi();
  client::Minecraft* client = clientInstance();
  if(client == nullptr || api.type(state, 1) != kLuaTString) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string id = luaString(state, 1, "");
  api.pushboolean(state, client->audio.play(id, readFloatArg(state, 2, 1.0f), readFloatArg(state, 3, 1.0f)) ? 1 : 0);
  return 1;
}
int luaPlaySoundAt(lua_State* state) {
  LuaApi& api = luaApi();
  client::Minecraft* client = clientInstance();
  if(client == nullptr || api.type(state, 1) != kLuaTString) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string id = luaString(state, 1, "");
  const float x = readFloatArg(state, 2, 0.0f);
  const float y = readFloatArg(state, 3, 0.0f);
  const float z = readFloatArg(state, 4, 0.0f);
  const float volume = readFloatArg(state, 5, 1.0f);
  const float pitch = readFloatArg(state, 6, 1.0f);
  api.pushboolean(state, client->audio.playAt(id, x, y, z, volume, pitch) ? 1 : 0);
  return 1;
}
int luaPlaySoundLoopAt(lua_State* state) {
  LuaApi& api = luaApi();
  client::Minecraft* client = clientInstance();
  if(client == nullptr || api.type(state, 1) != kLuaTString) {
    api.pushnil(state);
    return 1;
  }
  const std::string id = luaString(state, 1, "");
  const float x = readFloatArg(state, 2, 0.0f);
  const float y = readFloatArg(state, 3, 0.0f);
  const float z = readFloatArg(state, 4, 0.0f);
  const float volume = readFloatArg(state, 5, 1.0f);
  const float pitch = readFloatArg(state, 6, 1.0f);
  const std::string handle = client->audio.playLoopAt(id, x, y, z, volume, pitch);
  if(handle.empty()) {
    api.pushnil(state);
  } else {
    api.pushstring(state, handle.c_str());
  }
  return 1;
}
int luaStopSound(lua_State* state) {
  LuaApi& api = luaApi();
  client::Minecraft* client = clientInstance();
  if(client == nullptr || api.type(state, 1) != kLuaTString) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string handle = luaString(state, 1, "");
  if(handle.empty()) {
    api.pushboolean(state, 0);
    return 1;
  }
  client->audio.stop(handle);
  api.pushboolean(state, 1);
  return 1;
}
#endif
void installSoundApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  LuaApi& api = luaApi();
  api.createtable(state, 0, 5);
  bindModFunction(state, &mod, "register", luaRegisterSound);
  bindFunctions(state,
                {
                    {"play", luaPlaySound},
                    {"play_at", luaPlaySoundAt},
                    {"play_loop_at", luaPlaySoundLoopAt},
                    {"stop", luaStopSound},
                });
  api.setfield(state, -2, "sound");
#else
  (void)state;
  (void)mod;
#endif
}
} // namespace net::minecraft::mod::runtime
