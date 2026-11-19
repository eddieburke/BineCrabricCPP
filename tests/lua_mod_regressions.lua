local load_text = load
local load_file = loadfile
local original_package_path = package.path
local original_io, original_os, original_debug = io, os, debug
local callbacks = {}
minecraft = {
  _subscribe = function(name, _, callback)
    callbacks[name] = callback
    return true
  end,
  _register_block = function() return true end,
  _register_item = function() return true end,
  _register_shaped_recipe = function() return true end,
  _read_storage = function() end,
  _write_storage = function() return true end,
  asset_path = function() return ROOT .. "/mods/test" end,
  util = {},
}
assert(load_text(PRELUDE, "@minecraft/runtime.lua"))()
local parse = minecraft.util.parse_boolean
for _, value in ipairs({false, "false", "0", 0, "off", " NO "}) do
  assert(parse(value) == false, "false boolean lost: " .. tostring(value))
end
for _, value in ipairs({true, "true", "1", 1, "on", " YES "}) do
  assert(parse(value) == true, "true boolean lost: " .. tostring(value))
end
assert(parse(nil) == nil and parse("") == nil and parse("unknown") == nil)
local calls = 0
assert(minecraft.on("probe", { kind = {"a", "b"}, once = true }, function(event)
  calls = calls + 1
  event.handled = true
end))
assert(callbacks.probe({ kind = "c" }).handled == nil)
assert(callbacks.probe({ kind = "b" }).handled == true)
assert(callbacks.probe({ kind = "b" }).handled == nil and calls == 1)
assert(minecraft.on("predicate", { value = function(value) return value > 2 end }, function(event)
  event.handled = true
end))
assert(callbacks.predicate({ value = 2 }).handled == nil)
assert(callbacks.predicate({ value = 3 }).handled == true)
assert(io == nil and debug == nil and dofile == nil and loadfile == nil)
assert(require("os").execute == nil and package.loadlib == nil and package.cpath == "")
assert(not load(string.dump(function() end)))
package.path = original_package_path
io, os, debug = original_io, original_os, original_debug

local native_values, stored_values = {}, {}
local registered_entries
local fail_write = false
minecraft = {
  settings = {
    register = function(_, entries) registered_entries = entries end,
    get = function(key) return native_values[key] end,
    set = function(key, value) native_values[key] = value end,
  },
  storage = {
    read = function(path) return stored_values[path] end,
    write = function(path, value)
      if fail_write then return false end
      stored_values[path] = value
      return true
    end,
  },
}
local settings = assert(load_file(ROOT .. "/mods/lib/settings/init.lua"))()
local config = settings.define("test", {
  fields = {
    count = {type = "int", default = 6},
    implicit = {min = 2, max = 5, default = 3},
    zone = {type = "string", default = "GMT+0"},
  },
})
for _, entry in ipairs(registered_entries) do
  if entry.key == "count" then assert(entry.max == 2147483647) end
end
config.count = 12.9
assert(config.count == 12)
config.implicit = 100
assert(config.implicit == 5)
config.implicit = "invalid"
assert(config.implicit == 2)
config.zone = "GMT+9"
assert(config.zone == "GMT+9" and stored_values["settings/zone.txt"] == "GMT+9")
fail_write = true
assert(not pcall(function() config.zone = "GMT-5" end))
assert(config.zone == "GMT+9")

local item_defs = {
  [1] = {damageable = true, max_damage = 100, max_count = 1, stackable = false},
  [2] = {damageable = true, max_damage = 200, max_count = 1, stackable = false},
  [3] = {damageable = false, max_count = 64, stackable = true},
}
local screen_callbacks = {}
local cursor = {id = 0, count = 0}
minecraft = {
  items = {describe = function(id) return item_defs[id] end},
  gui = {},
  inventory = {
    cursor_get = function() return cursor end,
    cursor_set = function(value) cursor = value return true end,
    give = function() return true end,
  },
  screen = {open = function() end},
  on = function(_, options, callback) screen_callbacks[options.screen_id] = callback end,
}
local helper = assert(load_file(ROOT .. "/mods/repair_table/scripts/inventory_helper.lua"))()
package.loaded["scripts.inventory_helper"] = helper
local changes = 0
local slots = helper.slots({
  id = "test:slots",
  positions = {{x = 10, y = 10}},
  on_slot_change = function() changes = changes + 1 end,
})
slots.ctx.set(1, {id = 1, count = 1, damage = 10})
cursor = {id = 3, count = 64, damage = 0}
screen_callbacks["test:slots"]({phase = "init", width = 176, height = 72})
changes = 0
screen_callbacks["test:slots"]({phase = "mouse", x = 10, y = 10, button = 0})
assert(changes == 1, "one click must send one change notification")
assert(slots.ctx.get(1).id == 3 and slots.ctx.get(1).count == 64)
assert(cursor.id == 1 and cursor.count == 1)

local repair_ctx
local build_slots = helper.slots
helper.slots = function(spec)
  local screen = build_slots(spec)
  repair_ctx = screen.ctx
  return screen
end
assert(load_file(ROOT .. "/mods/repair_table/scripts/repair_screen.lua"))()
local repair_tick = screen_callbacks["repair_table:repair"]
for _, pair in ipairs({
  {{id = 1, count = 1, damage = 60}, {id = 2, count = 1, damage = 80}},
  {{id = 3, count = 1, damage = 0}, {id = 3, count = 1, damage = 0}},
  {{id = 1, count = 1, damage = 0}, {id = 1, count = 1, damage = 0}},
}) do
  repair_ctx.set(1, pair[1])
  repair_ctx.set(2, pair[2])
  repair_ctx.set(3, helper.stack.empty())
  repair_tick({phase = "tick"})
  assert(repair_ctx.get(1).id == pair[1].id and repair_ctx.get(1).count == pair[1].count)
  assert(repair_ctx.get(2).id == pair[2].id and repair_ctx.get(2).count == pair[2].count)
  assert(helper.stack.is_empty(repair_ctx.get(3)), "invalid repair produced output")
end
repair_ctx.set(1, {id = 1, count = 1, damage = 60})
repair_ctx.set(2, {id = 1, count = 1, damage = 70})
repair_tick({phase = "tick"})
assert(helper.stack.is_empty(repair_ctx.get(1)) and helper.stack.is_empty(repair_ctx.get(2)))
assert(repair_ctx.get(3).id == 1 and repair_ctx.get(3).count == 1 and repair_ctx.get(3).damage == 30)
repair_tick({phase = "tick"})
assert(repair_ctx.get(3).damage == 30 and repair_ctx.get(3).count == 1)

local registered_block, registered_recipe
package.loaded.config = {}
minecraft = {
  register_block = function(spec) registered_block = spec end,
  register_shaped_recipe = function(spec) registered_recipe = spec end,
  log = function() end,
}
assert(load_file(ROOT .. "/mods/stone_bricks/scripts/main.lua"))()
assert(registered_block.id == 98 and registered_block.model == nil)
assert(registered_recipe.output_block_id == 98 and registered_recipe.output_count == 4)
minecraft.model = {load = function() return 42 end}
assert(load_file(ROOT .. "/mods/stone_bricks/scripts/main.lua"))()
assert(registered_block.model == 42)
minecraft.model.load = function() return nil, "missing model" end
local ok, message = pcall(assert(load_file(ROOT .. "/mods/stone_bricks/scripts/main.lua")))
assert(not ok and message:find("missing model"))

local profile_callbacks = {}
local caption
minecraft = {
  util = {clamp = function(value, minimum, maximum) return math.max(minimum, math.min(maximum, value)) end},
  screen = {regions = {footer = "footer"}, add_button = function() end},
  gui = {
    draw_centered_text = function(x, y, width, text, color)
      assert(type(x) == "number" and type(y) == "number" and type(width) == "number")
      assert(type(text) == "string" and type(color) == "number")
      caption = {x, y, width, text, color}
    end,
    draw_slider = function() end,
  },
  on = function(name, _, callback) profile_callbacks[name] = callback end,
}
local profile_screen = assert(load_file(ROOT .. "/mods/world_profiles/scripts/settings_screen.lua"))()
local sliders, values = {}, {}
for index = 1, 8 do
  sliders[index] = {key = "field" .. index, min = 0, max = 1}
  values["field" .. index] = 0.5
end
profile_screen.register({id = "test:profiles", sliders = sliders, values = values})
profile_callbacks.screen_event({phase = "init", width = 320, height = 180})
profile_callbacks.screen_event({phase = "render", width = 320, height = 180})
assert(caption[1] == 0 and caption[3] == 320 and caption[4] == "Page 1 / 2")

local quad_batches = {}
minecraft = {
  util = {
    clamp = function(value, minimum, maximum)
      return math.max(minimum, math.min(maximum, value))
    end,
  },
  render = {
    quads = function(spec)
      quad_batches[#quad_batches + 1] = spec
      return math.floor(#spec.packed / 36)
    end,
  },
}
package.path = ROOT .. "/mods/realtime_sky/?.lua;" .. ROOT .. "/mods/realtime_sky/?/init.lua;" .. original_package_path
package.loaded["scripts.solar"] = nil
package.loaded["scripts.planets"] = nil
package.loaded["scripts.sky_render"] = nil
local solar = require("scripts.solar")
local planets = require("scripts.planets")
local sky_render = require("scripts.sky_render")

local prev_celestial = -1.0
local prev_alt = 90.0
local sunset_settings = { latitude = 40.71, longitude = -74.01, time_zone_id = "GMT-5" }
for step = 0, 20 do
  local utc_ms = 1794603600000.0 + step * 300000.0
  local f = solar.build_frame(sunset_settings, 0.0, utc_ms)
  assert(f.sun_altitude_deg < prev_alt, "sun altitude not decreasing at step " .. tostring(step))
  assert(f.celestial > prev_celestial, "celestial not increasing through sunset at step " .. tostring(step))
  prev_alt = f.sun_altitude_deg
  prev_celestial = f.celestial
end
assert(math.abs(solar.celestial_from_sun_altitude(0.0, 1.0) - 0.25) < 1.0e-6)
assert(math.abs(solar.celestial_from_sun_altitude(0.0, -1.0) - 0.75) < 1.0e-6)

local frame = {
  utc_millis = 1794600000000.0,
  sun_direction_x = 0.0,
  sun_direction_y = -0.3,
  sun_direction_z = 0.95,
  sun_altitude_deg = -15.0,
  moon_direction_x = 0.5,
  moon_direction_y = 0.6,
  moon_direction_z = 0.62,
  moon_illumination = 0.5,
}
local planet_settings = {
  planets_enabled = true,
  planet_daylight = true,
  planet_scale = 2.0,
  latitude = 43.2,
  longitude = -80.3833,
  planet_mercury = true,
  planet_venus = true,
  planet_mars = true,
  planet_jupiter = true,
  planet_saturn = true,
  planet_uranus = true,
  planet_neptune = true,
  planet_pluto = true,
}
local visible, count = planets.collect(planet_settings, frame, 1.0, nil)
assert(count >= 1, "expected at least one planet above horizon")
planet_settings.planet_mercury = false
planet_settings.planet_venus = false
planet_settings.planet_mars = false
planet_settings.planet_jupiter = false
planet_settings.planet_saturn = false
planet_settings.planet_uranus = false
planet_settings.planet_neptune = false
planet_settings.planet_pluto = false
local _, zero_count = planets.collect(planet_settings, frame, 1.0, nil)
assert(zero_count == 0)

sky_render.draw_planets({ eye_x = 10.0, eye_y = 64.0, eye_z = -5.0 }, frame, visible, count, 2.0)
assert(#quad_batches == 1)
local batch = quad_batches[1]
assert(batch.world_space == true and batch.blend == true and batch.cull == false)
assert(batch.depth_test == false and batch.depth_write == false)
assert(#batch.packed >= 36 and (#batch.packed % 36) == 0)

frame.sun_altitude_deg = 25.0
frame.sun_direction_y = 0.42
sky_render.draw_planets({ eye_x = 10.0, eye_y = 64.0, eye_z = -5.0 }, frame, visible, count, 2.0)
sky_render.draw_moon({ eye_x = 10.0, eye_y = 64.0, eye_z = -5.0 }, frame, 1.0, sky_render.MOON_MEAN_HALF_SIZE)
assert(#quad_batches == 3)
assert(quad_batches[2].blend == "additive")
assert(quad_batches[3].blend == "additive")
package.path = original_package_path
assert(load_file(ROOT .. "/tests/world_profiles_regressions.lua"))(load_file)
