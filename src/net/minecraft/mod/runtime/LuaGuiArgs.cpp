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
bool GuiDrawArgs::init(lua_State* state, int minPositional, int stringArg) {
  LuaApi& api = luaApi();
  state_ = state;
  table_ = api.type(state, 1) == kLuaTTable;
  if(table_) {
    rect_.x = static_cast<int>(luaFloatField(state, 1, "x", 0.0f));
    rect_.y = static_cast<int>(luaFloatField(state, 1, "y", 0.0f));
    rect_.w = static_cast<int>(luaFloatField(state, 1, "width", luaFloatField(state, 1, "w", 0.0f)));
    rect_.h = static_cast<int>(luaFloatField(state, 1, "height", luaFloatField(state, 1, "h", 0.0f)));
    return rect_.w > 0 && rect_.h > 0;
  }
  if(api.gettop(state) < minPositional || (stringArg > 0 && api.type(state, stringArg) != kLuaTString)) {
    return false;
  }
  rect_.x = luaIntArg(state, 1);
  rect_.y = luaIntArg(state, 2);
  rect_.w = luaIntArg(state, 3);
  rect_.h = luaIntArg(state, 4);
  return true;
}
std::string GuiDrawArgs::text(const char* field, const char* altField, int positional) const {
  if(table_) {
    return altField != nullptr ? luaStringField(state_, 1, field, luaStringField(state_, 1, altField, ""))
                               : luaStringField(state_, 1, field, "");
  }
  return luaString(state_, positional, "");
}
float GuiDrawArgs::number(const char* field, const char* altField, int positional, float fallback) const {
  if(table_) {
    return altField != nullptr ? luaFloatField(state_, 1, field, luaFloatField(state_, 1, altField, fallback))
                               : luaFloatField(state_, 1, field, fallback);
  }
  return luaFloatArg(state_, positional, fallback);
}
bool GuiDrawArgs::boolean(const char* field, const char* altField, int positional, bool fallback) const {
  if(table_) {
    return altField != nullptr ? luaBoolField(state_, 1, field, luaBoolField(state_, 1, altField, fallback))
                               : luaBoolField(state_, 1, field, fallback);
  }
  LuaApi& api = luaApi();
  return api.gettop(state_) < positional ? fallback : api.toboolean(state_, positional) != 0;
}
bool GuiDrawArgs::hovered(int positional) const {
  LuaApi& api = luaApi();
  if(table_) {
    api.getfield(state_, 1, "hovered");
    if(api.type(state_, -1) == kLuaTBoolean || api.type(state_, -1) == kLuaTNumber) {
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
  if(api.gettop(state_) >= positional + 1 && api.type(state_, positional) == kLuaTNumber &&
     api.type(state_, positional + 1) == kLuaTNumber) {
    return rect_.contains(luaIntArg(state_, positional), luaIntArg(state_, positional + 1));
  }
  return api.gettop(state_) >= positional && api.toboolean(state_, positional) != 0;
}
} // namespace net::minecraft::mod::runtime
