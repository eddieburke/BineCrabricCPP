#include "net/minecraft/mod/runtime/LuaGuiArgs.hpp"
#include <cmath>
#include <limits>
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
std::uint32_t luaArgb(lua_State* state, int index, std::uint32_t fallback) {
  int isNumber = 0;
  const double value = luaApi().tonumberx(state, index, &isNumber);
  if(isNumber == 0 || !std::isfinite(value)) {
    return fallback;
  }
  if(value < 0.0) {
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
  }
  return static_cast<std::uint32_t>(static_cast<std::uint64_t>(value));
}
bool GuiDrawArgs::init(lua_State* state) {
  LuaApi& api = luaApi();
  state_ = state;
  if(api.type(state, 1) != kLuaTTable) {
    return false;
  }
  rect_.x = static_cast<int>(luaFloatField(state, 1, "x", 0.0f));
  rect_.y = static_cast<int>(luaFloatField(state, 1, "y", 0.0f));
  rect_.w = static_cast<int>(luaFloatField(state, 1, "width", 0.0f));
  rect_.h = static_cast<int>(luaFloatField(state, 1, "height", 0.0f));
  return rect_.w > 0 && rect_.h > 0;
}
std::string GuiDrawArgs::text(const char* field) const {
  return luaStringField(state_, 1, field, "");
}
float GuiDrawArgs::number(const char* field, float fallback) const {
  return luaFloatField(state_, 1, field, fallback);
}
bool GuiDrawArgs::boolean(const char* field, bool fallback) const {
  return luaBoolField(state_, 1, field, fallback);
}
bool GuiDrawArgs::hovered() const {
  LuaApi& api = luaApi();
  api.getfield(state_, 1, "hovered");
  if(api.type(state_, -1) == kLuaTBoolean) {
    const bool value = api.toboolean(state_, -1) != 0;
    pop(state_, 1);
    return value;
  }
  pop(state_, 1);
  const float mouseX = luaFloatField(state_, 1, "mouse_x", std::numeric_limits<float>::quiet_NaN());
  const float mouseY = luaFloatField(state_, 1, "mouse_y", std::numeric_limits<float>::quiet_NaN());
  if(std::isfinite(mouseX) && std::isfinite(mouseY)) {
    return rect_.contains(static_cast<int>(mouseX), static_cast<int>(mouseY));
  }
  return false;
}
} // namespace net::minecraft::mod::runtime
