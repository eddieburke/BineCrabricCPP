#pragma once
#include <cstddef>
struct lua_State;
namespace net::minecraft {
class Nbt;
namespace mod::lua {
void pushNbtValue(lua_State* state, const Nbt& value, std::size_t depth = 0);
// Converts a lua value (boolean/number/string/table) at the given stack index into
// an Nbt value. Tables become compounds keyed by their string keys. Non-table,
// non-scalar values degrade to an empty compound.
Nbt luaValueToNbt(lua_State* state, int index);
} // namespace mod::lua
} // namespace net::minecraft
