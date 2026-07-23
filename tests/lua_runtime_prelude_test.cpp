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
TEST(LuaRuntimePrelude, RequireModuleResolution) {
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
 api.pushstring(state, "testmod");
 api.setfield(state, -2, "mod_id");
 api.setglobal(state, "minecraft");
 const std::string_view source = net::minecraft::mod::lua::kRuntimePrelude;
 ASSERT_EQ(api.loadbufferx(state, source.data(), source.size(), "@minecraft/runtime.lua", "t"),
           net::minecraft::mod::lua::kLuaOk);
 ASSERT_EQ(api.pcallk(state, 0, 0, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk);
 net::minecraft::mod::lua::getglobal(state, "package");
 api.getfield(state, -1, "preload");
 std::string code1 = "return { val = 42 }";
 api.loadbufferx(state, code1.data(), code1.size(), "config", "t");
 api.setfield(state, -2, "config");
 net::minecraft::mod::lua::getglobal(state, "require");
 api.pushstring(state, "testmod.config");
 ASSERT_EQ(api.pcallk(state, 1, 1, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk);
 api.getfield(state, -1, "val");
 EXPECT_EQ(api.tointegerx(state, -1, nullptr), 42);
 net::minecraft::mod::lua::pop(state, 3);
 net::minecraft::mod::lua::getglobal(state, "package");
 api.getfield(state, -1, "preload");
 std::string code2 = "return { val = 99 }";
 api.loadbufferx(state, code2.data(), code2.size(), "testmod.other", "t");
 api.setfield(state, -2, "testmod.other");
 net::minecraft::mod::lua::getglobal(state, "require");
 api.pushstring(state, "testmod.other");
 ASSERT_EQ(api.pcallk(state, 1, 1, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk);
 api.getfield(state, -1, "val");
 EXPECT_EQ(api.tointegerx(state, -1, nullptr), 99);
 net::minecraft::mod::lua::pop(state, 3);
 net::minecraft::mod::lua::getglobal(state, "package");
 api.getfield(state, -1, "loaded");
 api.pushnil(state);
 api.setfield(state, -2, "config");
 net::minecraft::mod::lua::pop(state, 1);
 net::minecraft::mod::lua::getglobal(state, "package");
 api.getfield(state, -1, "preload");
 std::string code3 = "error('internal failure')";
 api.loadbufferx(state, code3.data(), code3.size(), "config", "t");
 api.setfield(state, -2, "config");
 net::minecraft::mod::lua::getglobal(state, "require");
 api.pushstring(state, "testmod.config");
 EXPECT_NE(api.pcallk(state, 1, 1, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk);
 std::string err = api.tolstring(state, -1, nullptr);
 EXPECT_TRUE(err.find("internal failure") != std::string::npos) << err;
 api.close(state);
}
} // namespace
