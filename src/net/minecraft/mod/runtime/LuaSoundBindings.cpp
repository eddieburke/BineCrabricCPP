#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/platform/audio/AudioEngine.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaBindings.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
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
 LuaArgs args(state);
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 std::string id;
 std::string path;
 if(mod == nullptr || !args.string(1, id) || !args.string(2, path)) {
  return args.fail("minecraft.sound.register expects (id, path, kind?)");
 }
 std::string kind = "effect";
 if(!args.optionalString(3, kind)) {
  return args.fail("minecraft.sound.register expects kind to be a string");
 }
 if(id.empty() || path.empty()) {
  return args.fail("sound id and path are required");
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
 LuaArgs args(state);
 client::Minecraft* client = clientInstance();
 std::string id;
 if(!args.string(1, id)) {
  return args.fail("minecraft.sound.play expects (id, volume?, pitch?)");
 }
 double volume = 1.0;
 double pitch = 1.0;
 if(!args.optionalNumber(2, volume) || !args.optionalNumber(3, pitch)) {
  return args.fail("minecraft.sound.play expects numeric volume and pitch");
 }
 if(client == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 api.pushboolean(state, client->audio.play(id, static_cast<float>(volume), static_cast<float>(pitch)) ? 1 : 0);
 return 1;
}
int luaPlaySoundAt(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 client::Minecraft* client = clientInstance();
 std::string id;
 if(!args.string(1, id)) {
  return args.fail("minecraft.sound.play_at expects (id, x?, y?, z?, volume?, pitch?)");
 }
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double volume = 1.0;
 double pitch = 1.0;
 if(!args.optionalNumber(2, x) || !args.optionalNumber(3, y) || !args.optionalNumber(4, z) ||
    !args.optionalNumber(5, volume) || !args.optionalNumber(6, pitch)) {
  return args.fail("minecraft.sound.play_at expects numeric coordinates, volume, and pitch");
 }
 if(client == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 api.pushboolean(state,
                 client->audio.playAt(id,
                                      static_cast<float>(x),
                                      static_cast<float>(y),
                                      static_cast<float>(z),
                                      static_cast<float>(volume),
                                      static_cast<float>(pitch))
                     ? 1
                     : 0);
 return 1;
}
int luaPlaySoundLoopAt(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 client::Minecraft* client = clientInstance();
 std::string id;
 if(!args.string(1, id)) {
  return args.fail("minecraft.sound.play_loop_at expects (id, x?, y?, z?, volume?, pitch?)");
 }
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double volume = 1.0;
 double pitch = 1.0;
 if(!args.optionalNumber(2, x) || !args.optionalNumber(3, y) || !args.optionalNumber(4, z) ||
    !args.optionalNumber(5, volume) || !args.optionalNumber(6, pitch)) {
  return args.fail("minecraft.sound.play_loop_at expects numeric coordinates, volume, and pitch");
 }
 if(client == nullptr) {
  api.pushnil(state);
  return 1;
 }
 const std::string handle = client->audio.playLoopAt(id,
                                                     static_cast<float>(x),
                                                     static_cast<float>(y),
                                                     static_cast<float>(z),
                                                     static_cast<float>(volume),
                                                     static_cast<float>(pitch));
 if(handle.empty()) {
  api.pushnil(state);
 } else {
  api.pushstring(state, handle.c_str());
 }
 return 1;
}
int luaStopSound(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 client::Minecraft* client = clientInstance();
 std::string handle;
 if(!args.string(1, handle)) {
  return args.fail("minecraft.sound.stop expects (handle)");
 }
 if(client == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
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
