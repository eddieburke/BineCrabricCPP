#pragma once
#include <cstdint>
#include <string>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
struct lua_State;
namespace net::minecraft::mod::runtime {
struct GuiRect {
 int x = 0;
 int y = 0;
 int w = 0;
 int h = 0;
 [[nodiscard]] bool contains(int mouseX, int mouseY) const noexcept {
  return mouseX >= x && mouseY >= y && mouseX < x + w && mouseY < y + h;
 }
};
// Packed 0xAARRGGBB color arg; accepts negative (signed) encodings from Lua.
[[nodiscard]] std::uint32_t luaArgb(lua_State* state, int index, std::uint32_t fallback = 0xFFFFFFFFU);
class GuiDrawArgs {
 public:
 [[nodiscard]] bool init(lua_State* state);
 [[nodiscard]] const GuiRect& rect() const noexcept {
  return rect_;
 }
 [[nodiscard]] std::string text(const char* field) const;
 [[nodiscard]] float number(const char* field, float fallback) const;
 [[nodiscard]] bool boolean(const char* field, bool fallback) const;
 [[nodiscard]] bool hovered() const;

 private:
 lua_State* state_ = nullptr;
 GuiRect rect_;
};
} // namespace net::minecraft::mod::runtime
