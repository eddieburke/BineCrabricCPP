#include <gtest/gtest.h>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaRuntimePrelude.hpp"
namespace {
int luaNoop(lua_State*) {
 return 0;
}
int luaCurrentDirectory(lua_State* state) {
 net::minecraft::mod::lua::luaApi().pushstring(state, ".");
 return 1;
}
TEST(LuaRuntimePrelude, Parses) {
 net::minecraft::mod::lua::LuaApi& api = net::minecraft::mod::lua::luaApi();
 if(!api.ready()) {
  GTEST_SKIP() << "Lua runtime unavailable";
 }
 lua_State* state = api.newstate();
 ASSERT_NE(state, nullptr);
 const std::string_view source = net::minecraft::mod::lua::kRuntimePrelude;
 EXPECT_EQ(api.loadbufferx(state, source.data(), source.size(), "@minecraft/runtime.lua", "t"),
           net::minecraft::mod::lua::kLuaOk);
 api.close(state);
}
TEST(LuaRuntimePrelude, ExecutesWithoutClientRenderApi) {
 net::minecraft::mod::lua::LuaApi& api = net::minecraft::mod::lua::luaApi();
 if(!api.ready()) {
  GTEST_SKIP() << "Lua runtime unavailable";
 }
 lua_State* state = api.newstate();
 ASSERT_NE(state, nullptr);
 api.openlibs(state);
 api.createtable(state, 0, 8);
 for(const char* name : {"_subscribe", "_register_block", "_register_item", "_register_shaped_recipe",
                         "_read_storage", "_write_storage"}) {
  api.pushcclosure(state, luaNoop, 0);
  api.setfield(state, -2, name);
 }
 api.createtable(state, 0, 8);
 api.setfield(state, -2, "util");
 api.createtable(state, 0, 8);
 api.setfield(state, -2, "world");
 api.pushcclosure(state, luaCurrentDirectory, 0);
 api.setfield(state, -2, "asset_path");
 api.setglobal(state, "minecraft");
 const std::string_view source = net::minecraft::mod::lua::kRuntimePrelude;
 ASSERT_EQ(api.loadbufferx(state, source.data(), source.size(), "@minecraft/runtime.lua", "t"),
           net::minecraft::mod::lua::kLuaOk);
 EXPECT_EQ(api.pcallk(state, 0, 0, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 api.close(state);
}
} // namespace
