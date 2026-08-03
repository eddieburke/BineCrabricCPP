#pragma once
struct lua_State;
namespace net::minecraft::mod::runtime {
void installRenderApi(lua_State* state);
[[nodiscard]] bool itemModelRenderOverrideActive();
void setItemModelRenderOverride(bool enabled);
} // namespace net::minecraft::mod::runtime
