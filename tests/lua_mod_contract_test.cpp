#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaRuntimePrelude.hpp"
namespace {
TEST(LuaModContracts, SharedLibrariesAndBundledMods) {
 auto& api = net::minecraft::mod::lua::luaApi();
 if(!api.ready()) {
  GTEST_SKIP() << "Lua runtime unavailable";
 }
 lua_State* state = api.newstate();
 ASSERT_NE(state, nullptr);
 api.openlibs(state);
 const std::string root = std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR).generic_string();
 const std::string bootstrap = "ROOT = [=[" + root + "]=]\nPRELUDE = [====[" +
     std::string(net::minecraft::mod::lua::kRuntimePrelude) + "]====]\n";
 int status = api.loadbufferx(state, bootstrap.data(), bootstrap.size(), "@mod_contract_bootstrap", "t");
 if(status == net::minecraft::mod::lua::kLuaOk) {
  status = api.pcallk(state, 0, 0, 0, 0, nullptr);
 }
 if(status == net::minecraft::mod::lua::kLuaOk) {
  const std::string path = (std::filesystem::path(root) / "tests/lua_mod_regressions.lua").string();
  status = api.loadfilex(state, path.c_str(), "t");
  if(status == net::minecraft::mod::lua::kLuaOk) {
   status = api.pcallk(state, 0, 0, 0, 0, nullptr);
  }
 }
 EXPECT_EQ(status, net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 api.close(state);
}
}
