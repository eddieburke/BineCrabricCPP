#pragma once
struct lua_State;
namespace net::minecraft::mod::runtime {
void installRenderApi(lua_State* state);
// When enabled, the native dropped-item (ItemEntity) sprite renderer is
// suppressed so a Lua mod can draw its own 3D physics-driven model instead.
[[nodiscard]] bool itemModelRenderOverrideActive();
void setItemModelRenderOverride(bool enabled);
} // namespace net::minecraft::mod::runtime
