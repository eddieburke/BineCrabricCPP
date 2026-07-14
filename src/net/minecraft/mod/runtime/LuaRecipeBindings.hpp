#pragma once
#include "net/minecraft/mod/runtime/ModHost.hpp"
struct lua_State;
namespace net::minecraft::mod::runtime {
void installRecipeApi(lua_State* state, ModHost::LoadedLuaMod& mod);
}
