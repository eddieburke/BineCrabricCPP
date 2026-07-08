#pragma once
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include <cstdint>
#include <string>
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
// Unified reader for gui.draw_* calls that accept either a single options
// table or positional args: rect at 1..4, then text/value/flags.
class GuiDrawArgs {
public:
  // Parses the rect (table fields x/y/width|w/height|h, or positional 1..4).
  // For the positional form, requires gettop >= minPositional and a string at
  // stringArg (0 = no string requirement). Returns false when malformed.
  [[nodiscard]] bool init(lua_State* state, int minPositional, int stringArg);
  [[nodiscard]] bool fromTable() const noexcept {
    return table_;
  }
  [[nodiscard]] const GuiRect& rect() const noexcept {
    return rect_;
  }
  [[nodiscard]] std::string text(const char* field, const char* altField, int positional) const;
  [[nodiscard]] float number(const char* field, const char* altField, int positional, float fallback) const;
  [[nodiscard]] bool boolean(const char* field, const char* altField, int positional, bool fallback) const;
  // Table form: "hovered" field, else mouse_x/mouse_y containment. Positional
  // form: numeric mouse pair at positional/positional+1, else bool at positional.
  [[nodiscard]] bool hovered(int positional) const;

private:
  lua_State* state_ = nullptr;
  GuiRect rect_;
  bool table_ = false;
};
} // namespace net::minecraft::mod::runtime
