#pragma once
#include "net/minecraft/mod/runtime/ModHost.hpp"
struct lua_State;

namespace net::minecraft::mod::lua {
void installGenericModApi(lua_State* state, net::minecraft::mod::runtime::ModHost::LoadedLuaMod& mod);
}  // namespace net::minecraft::mod::lua
