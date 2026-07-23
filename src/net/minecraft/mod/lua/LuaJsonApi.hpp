#pragma once
struct lua_State;
namespace net::minecraft::mod::lua {
int luaJsonEncode(lua_State* state);
int luaJsonDecode(lua_State* state);
void* luaJsonNull();
} // namespace net::minecraft::mod::lua
