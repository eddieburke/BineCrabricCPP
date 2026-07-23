#pragma once
struct lua_State;
namespace net::minecraft::mod::runtime {
void installBox3dApi(lua_State* state);
void setBox3dPreloaded(lua_State* state);
} // namespace net::minecraft::mod::runtime
