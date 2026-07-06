#include "net/minecraft/mod/lua/LuaNbtCodec.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/nbt/Nbt.hpp"
#include <algorithm>
#include <cstdint>
namespace net::minecraft::mod::lua {
void pushNbtValue(lua_State* state, const Nbt& value, std::size_t depth) {
  LuaApi& api = luaApi();
  if(depth > 256) {
    api.pushnil(state);
    return;
  }
  switch(value.type()) {
  case Nbt::Type::End:
    api.pushnil(state);
    return;
  case Nbt::Type::Byte:
    api.pushinteger(state, value.asByte());
    return;
  case Nbt::Type::Short:
    api.pushinteger(state, value.asShort());
    return;
  case Nbt::Type::Int:
    api.pushinteger(state, value.asInt());
    return;
  case Nbt::Type::Long:
    api.pushinteger(state, value.asLong());
    return;
  case Nbt::Type::Float:
    api.pushnumber(state, value.asFloat());
    return;
  case Nbt::Type::Double:
    api.pushnumber(state, value.asDouble());
    return;
  case Nbt::Type::String: {
    const std::string& text = value.asString();
    api.pushlstring(state, text.data(), text.size());
    return;
  }
  case Nbt::Type::ByteArray: {
    const Nbt::ByteArray& bytes = value.asByteArray();
    api.pushlstring(state, reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return;
  }
  case Nbt::Type::List: {
    const Nbt::List& list = value.asList();
    api.createtable(state, static_cast<int>(std::min<std::size_t>(list.size(), INT32_MAX)), 0);
    for(std::size_t index = 0; index < list.size(); ++index) {
      pushNbtValue(state, list[index], depth + 1);
      api.rawseti(state, -2, static_cast<long long>(index + 1));
    }
    return;
  }
  case Nbt::Type::Compound:
    api.createtable(state, 0, static_cast<int>(std::min<std::size_t>(value.asCompound().size(), INT32_MAX)));
    for(const auto& [key, child] : value.asCompound()) {
      pushNbtValue(state, child, depth + 1);
      api.setfield(state, -2, key.c_str());
    }
    return;
  case Nbt::Type::IntArray: {
    const Nbt::IntArray& values = value.asIntArray();
    api.createtable(state, static_cast<int>(std::min<std::size_t>(values.size(), INT32_MAX)), 0);
    for(std::size_t index = 0; index < values.size(); ++index) {
      api.pushinteger(state, values[index]);
      api.rawseti(state, -2, static_cast<long long>(index + 1));
    }
    return;
  }
  case Nbt::Type::LongArray: {
    const Nbt::LongArray& values = value.asLongArray();
    api.createtable(state, static_cast<int>(std::min<std::size_t>(values.size(), INT32_MAX)), 0);
    for(std::size_t index = 0; index < values.size(); ++index) {
      api.pushinteger(state, values[index]);
      api.rawseti(state, -2, static_cast<long long>(index + 1));
    }
    return;
  }
  }
  api.pushnil(state);
}
} // namespace net::minecraft::mod::lua
