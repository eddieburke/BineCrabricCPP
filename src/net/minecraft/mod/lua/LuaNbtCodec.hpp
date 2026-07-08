#pragma once
#include <cstddef>
struct lua_State;

namespace net::minecraft {
class Nbt;

namespace mod::lua {
void pushNbtValue(lua_State* state, const Nbt& value, std::size_t depth = 0);
}
}  // namespace net::minecraft
