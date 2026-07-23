#pragma once
#include "net/minecraft/mod/runtime/ModHost.hpp"
struct lua_State;
namespace net::minecraft::mod::runtime {
void installBlockApi(lua_State* state, ModHost::LoadedLuaMod& mod);
void installItemApi(lua_State* state, ModHost::LoadedLuaMod& mod);
void installRecipeApi(lua_State* state, ModHost::LoadedLuaMod& mod);
void installCoreApi(lua_State* state, ModHost::LoadedLuaMod& mod);
void installWorldApi(lua_State* state, ModHost::LoadedLuaMod& mod);
void pushChunkObject(lua_State* state);
#ifdef MINECRAFT_NATIVE_EXPORTS
void installTextureApi(lua_State* state);
void installSoundApi(lua_State* state, ModHost::LoadedLuaMod& mod);
void installInventoryApi(lua_State* state);
#endif
} // namespace net::minecraft::mod::runtime
