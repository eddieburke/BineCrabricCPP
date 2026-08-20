#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
namespace {
TEST(LuaSettingsModule, PersistsStringsAndRegistersIntegers) {
 auto& api = net::minecraft::mod::lua::luaApi();
 if(!api.ready()) {
  GTEST_SKIP() << "Lua runtime unavailable";
 }
 lua_State* state = api.newstate();
 ASSERT_NE(state, nullptr);
 api.openlibs(state);
 const std::string bootstrap = R"lua(
native_values = {}
stored_values = { ["settings/zone.txt"] = "GMT-5" }
registered_entries = nil
minecraft = {
  settings = {
    register = function(_, entries) registered_entries = entries end,
    get = function(key) return native_values[key] end,
    set = function(key, value) native_values[key] = value end,
  },
  storage = {
    read = function(path) return stored_values[path] end,
    write = function(path, value) stored_values[path] = value return true end,
  },
}
)lua";
 ASSERT_EQ(api.loadbufferx(state, bootstrap.data(), bootstrap.size(), "@settings_test_bootstrap.lua", "t"),
           net::minecraft::mod::lua::kLuaOk);
 ASSERT_EQ(api.pcallk(state, 0, 0, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 const std::filesystem::path modulePath =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "mods" / "lib" / "settings" / "init.lua";
 const std::string modulePathText = modulePath.string();
 ASSERT_EQ(api.loadfilex(state, modulePathText.c_str(), "t"), net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 ASSERT_EQ(api.pcallk(state, 0, 1, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 api.setglobal(state, "settings_module");
 const std::string assertions = R"lua(
local config = settings_module.define("test", {
  name = "Test",
  fields = {
    enabled = { type = "bool", default = true },
    count = { type = "int", min = 1, max = 8, default = 6 },
    zone = { type = "string", default = "GMT+0" },
  },
})
assert(#registered_entries == 2)
local count_entry = nil
for _, entry in ipairs(registered_entries) do
  assert(entry.key ~= "zone")
  if entry.key == "count" then count_entry = entry end
end
assert(count_entry ~= nil)
assert(count_entry.kind == "slider")
assert(count_entry.integer == true)
assert(count_entry.step == 1)
assert(count_entry.decimals == 0)
assert(config.zone == "GMT-5")
config.zone = "GMT+9:30"
assert(config.zone == "GMT+9:30")
assert(stored_values["settings/zone.txt"] == "GMT+9:30")
config.count = 3.9
assert(native_values["test.count"] == 3)
)lua";
 ASSERT_EQ(api.loadbufferx(state, assertions.data(), assertions.size(), "@settings_test_assertions.lua", "t"),
           net::minecraft::mod::lua::kLuaOk);
 ASSERT_EQ(api.pcallk(state, 0, 0, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 api.close(state);
}
TEST(LuaSettingsModule, RealtimeSkySourcesParse) {
 auto& api = net::minecraft::mod::lua::luaApi();
 if(!api.ready()) {
  GTEST_SKIP() << "Lua runtime unavailable";
 }
 lua_State* state = api.newstate();
 ASSERT_NE(state, nullptr);
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "mods" / "realtime_sky";
 for(const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
  if(!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
  const std::string path = entry.path().string();
  EXPECT_EQ(api.loadfilex(state, path.c_str(), "t"), net::minecraft::mod::lua::kLuaOk)
      << path << ": " << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
  api.settop(state, 0);
 }
 api.close(state);
}
TEST(LuaSettingsModule, RealtimeSkyRestoresExpectedWorldClockWhenDisabled) {
 auto& api = net::minecraft::mod::lua::luaApi();
 if(!api.ready()) {
  GTEST_SKIP() << "Lua runtime unavailable";
 }
 lua_State* state = api.newstate();
 ASSERT_NE(state, nullptr);
 api.openlibs(state);
 const std::string bootstrap = R"lua(
world_time = 1000
callbacks = {}
world_settings_button = nil
save_settings_callback = nil
config = {
  enabled = true,
  drive_sun = true,
  time_zone_id = "GMT+0",
  latitude = 0,
  longitude = 0,
}
package.preload["config"] = function() return config end
package.preload["scripts.solar"] = function()
  return {
    build_frame = function() return { day_tick = 6000 } end,
    time_zone_info = function() return { offset_minutes = 0 } end,
  }
end
package.preload["scripts.sky_render"] = function() return {} end
package.preload["scripts.places"] = function() return {} end
package.preload["scripts.globe_ui"] = function() return {} end
package.preload["scripts.settings_ui"] = function()
  return { register = function(_, save_fn) save_settings_callback = save_fn end }
end
minecraft = {
  util = { clamp = function(value, minimum, maximum)
    return math.max(minimum, math.min(maximum, value))
  end },
  screen = {
    ids = { world_settings = "world_settings", mod_settings = "mod_settings" },
    regions = { footer = "footer" },
  },
  time = { utc_millis = function() return 0 end },
  world = {
    get_time = function() return world_time end,
    set_time = function(value) world_time = value return true end,
  },
  on = function(name, options, callback)
    callbacks[name] = callbacks[name] or {}
    callbacks[name][#callbacks[name] + 1] = { options = options, callback = callback }
  end,
}
)lua";
 ASSERT_EQ(api.loadbufferx(state, bootstrap.data(), bootstrap.size(), "@realtime_restore_bootstrap.lua", "t"),
           net::minecraft::mod::lua::kLuaOk);
 ASSERT_EQ(api.pcallk(state, 0, 0, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 const std::filesystem::path hooksPath =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "mods" / "realtime_sky" / "scripts" / "hooks.lua";
 const std::string hooksPathText = hooksPath.string();
 ASSERT_EQ(api.loadfilex(state, hooksPathText.c_str(), "t"), net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 ASSERT_EQ(api.pcallk(state, 0, 1, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 api.setglobal(state, "hooks_module");
 const std::string assertions = R"lua(
hooks_module.register()
local ui = {
  add_stacked_centered_button = function(_, _, click)
    world_settings_button = click
  end,
}
callbacks.screen_ui[1].callback({ ui = ui })
callbacks.world_tick[1].callback({})
assert(world_time == 6000)
world_time = world_time + 1
callbacks.world_tick[1].callback({})
assert(world_time == 6000)
world_settings_button()
assert(config.enabled == false)
assert(world_time == 1001)
world_settings_button()
world_time = world_time + 1
callbacks.world_tick[1].callback({})
assert(world_time == 6000)
config.drive_sun = false
save_settings_callback()
assert(world_time == 1002)
)lua";
 ASSERT_EQ(api.loadbufferx(state, assertions.data(), assertions.size(), "@realtime_restore_assertions.lua", "t"),
           net::minecraft::mod::lua::kLuaOk);
 ASSERT_EQ(api.pcallk(state, 0, 0, 0, 0, nullptr), net::minecraft::mod::lua::kLuaOk)
     << net::minecraft::mod::lua::luaString(state, -1, "unknown Lua error");
 api.close(state);
}
}
