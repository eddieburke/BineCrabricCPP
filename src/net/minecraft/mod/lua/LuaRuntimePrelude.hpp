#pragma once
#include <string_view>
namespace net::minecraft::mod::lua {
// Lua owns composition above the narrow native subscription primitive. Keeping
// this as Lua makes filters and convenience APIs reusable without growing a
// bespoke C++ registration function for every kind of mod.
inline constexpr std::string_view kRuntimePrelude = R"lua(
local native_subscribe = assert(minecraft._subscribe, "native Lua event bridge is unavailable")
local native_register_block = assert(minecraft._register_block, "native Lua block registry is unavailable")
local native_register_shaped_recipe =
  assert(minecraft._register_shaped_recipe, "native Lua recipe registry is unavailable")
local native_read_storage = assert(minecraft._read_storage, "native Lua storage reader is unavailable")
local native_write_storage = assert(minecraft._write_storage, "native Lua storage writer is unavailable")

-- Runtime mods get deterministic Lua modules and mod-scoped persistence, not
-- ambient process/filesystem access.
local safe_os = {
  clock = os.clock,
  date = os.date,
  difftime = os.difftime,
  time = os.time,
}
os = safe_os
io = nil
debug = nil
dofile = nil
loadfile = nil
package.cpath = ""
package.loadlib = nil
package.searchers[4] = nil
package.searchers[3] = nil

local function readonly_constants(values)
  return setmetatable({}, {
    __index = values,
    __newindex = function()
      error("Minecraft constants are read-only", 2)
    end,
    __pairs = function()
      return next, values, nil
    end,
    __metatable = false,
  })
end

local function named_constants(names)
  local values = {}
  for _, name in ipairs(names) do
    values[name] = name
  end
  return readonly_constants(values)
end

minecraft.events = named_constants({
  "attack_damage", "block_interact", "chunk_generation", "client_tick", "create_world",
  "entity_interact", "fov", "key_press", "mouse_button", "player_travel", "screen_event",
  "screen_region", "screen_ui", "tick_rate", "world_color", "world_render",
  "world_open", "world_spawn_search", "world_start", "world_tick",
})

minecraft.lifecycle = named_constants({
  "bootstrap_starting", "biome_registration", "block_registration", "block_registry_finalize",
  "item_registration", "block_item_registration", "smelting_recipe_registration",
  "crafting_recipe_registration", "entity_registration", "block_entity_registration",
  "fuel_registration", "client_renderer_registration", "particle_registration", "frozen",
})

minecraft.generation = {
  stages = named_constants({ "terrain", "surface", "carver", "features" }),
  moments = named_constants({ "before", "after" }),
}

minecraft.render.stages = named_constants({ "sky", "stars", "clouds" })
minecraft.render.moments = named_constants({ "before", "after" })
minecraft.colors = named_constants({ "sky", "fog" })

minecraft.keys = readonly_constants({
  escape = 1,
  enter = 28,
  space = 57,
  up = 200,
  down = 208,
})

local reserved_options = {
  once = true,
  priority = true,
  when = true,
}

local function option_matches(actual, expected)
  if type(expected) == "function" then
    return not not expected(actual)
  end
  if type(expected) ~= "table" then
    return actual == expected
  end
  if actual ~= nil and expected[actual] ~= nil then
    return not not expected[actual]
  end
  for _, candidate in ipairs(expected) do
    if actual == candidate then
      return true
    end
  end
  return false
end

local function event_matches(event, options)
  for key, expected in pairs(options) do
    if not reserved_options[key] and not option_matches(event[key], expected) then
      return false
    end
  end
  return options.when == nil or not not options.when(event)
end

-- Supported forms:
--   minecraft.on(event, callback, priority?)       (compatibility)
--   minecraft.on(event, options, callback)         (preferred)
-- Every non-reserved option is an event-field filter. A filter may be a scalar,
-- an array/set of accepted values, or a predicate.
function minecraft.on(event_name, options, callback)
  if type(options) == "function" then
    return native_subscribe(event_name, options, tonumber(callback) or 0)
  end
  assert(type(options) == "table", "minecraft.on options must be a table")
  assert(type(callback) == "function", "minecraft.on callback must be a function")

  local active = true
  local cpp_filter = {}
  for key, expected in pairs(options) do
    if key == "screen_id" or key == "region" or key == "stage" or key == "moment" then
      if type(expected) == "string" then
        cpp_filter[key] = expected
      end
    end
  end
  local function filtered_callback(event)
    if not active or not event_matches(event, options) then
      return event
    end
    local result = callback(event)
    if options.once then
      active = false
    end
    return result or event
  end
  return native_subscribe(event_name, filtered_callback, tonumber(options.priority) or 0, cpp_filter)
end

-- Block behavior is declared with the block instead of forcing every mod to
-- rediscover its own block inside a global interaction callback.
function minecraft.register_block(spec)
  local ok, error_message = native_register_block(spec)
  assert(ok, error_message or "block registration failed")
  if type(spec.on_use) == "function" then
    minecraft.on(minecraft.events.block_interact, {
      block_id = assert(spec.id, "registered block requires an id"),
      right_click = true,
      priority = spec.behavior_priority or 0,
    }, function(event)
      local result = spec.on_use(event)
      if event.handled then
        event.canceled = true
      end
      return result
    end)
  end
end

function minecraft.register_shaped_recipe(spec)
  local ok, error_message = native_register_shaped_recipe(spec)
  assert(ok, error_message or "shaped recipe registration failed")
end

minecraft.util = {}
minecraft.astronomy = {}

function minecraft.util.clamp(value, minimum, maximum)
  return math.max(minimum, math.min(maximum, value))
end

-- Converts J2000 right ascension/declination to local azimuth/altitude.
-- Longitude is positive east; azimuth is degrees east from north.
function minecraft.astronomy.horizontal_from_equatorial(
    ra_hours, dec_degrees, utc_millis, latitude_degrees, longitude_degrees)
  local days = utc_millis / 86400000.0 - 10957.5
  local epoch_year = 2000.0 + days / 365.2422
  local correction = math.rad(3.82394e-5 * (365.2422 * (epoch_year - 2000.0) - days))
  local dec = math.rad(dec_degrees)
  local cos_dec = math.cos(dec)
  local ra = math.rad(ra_hours * 15.0) + (math.abs(cos_dec) < 1.0e-8 and correction
    or correction / cos_dec)
  local mean_anomaly = (356.0470 + 0.9856002585 * days) % 360.0
  local solar_perihelion = 282.9404 + 4.70935e-5 * days
  local utc_hours = (utc_millis % 86400000.0) / 3600000.0
  local lst = math.rad((mean_anomaly + solar_perihelion + 180.0 +
    utc_hours * 15.0 + longitude_degrees) % 360.0)
  local latitude = math.rad(minecraft.util.clamp(latitude_degrees, -90.0, 90.0))
  local hour_angle = lst - ra
  local sin_altitude = minecraft.util.clamp(
    math.sin(latitude) * math.sin(dec) +
    math.cos(latitude) * math.cos(dec) * math.cos(hour_angle), -1.0, 1.0)
  local altitude = math.asin(sin_altitude)
  local azimuth_south = math.atan(
    math.sin(hour_angle),
    math.cos(hour_angle) * math.sin(latitude) - math.tan(dec) * math.cos(latitude))
  return (math.deg(azimuth_south) + 180.0) % 360.0, math.deg(altitude)
end

function minecraft.util.trim(value)
  return tostring(value or ""):match("^%s*(.-)%s*$") or ""
end

function minecraft.util.in_rect(x, y, left, top, width, height)
  return x >= left and x < left + width and y >= top and y < top + height
end

function minecraft.util.real_world(event)
  return event ~= nil and event.mod_generation ~= false
end

function minecraft.util.resolve_seed(text)
  return minecraft._resolve_seed(text or "")
end

function minecraft.util.json_encode(value)
  local encoded, err = minecraft._json_encode(value)
  if encoded == nil then
    error(err or "json_encode failed", 2)
  end
  return encoded
end

function minecraft.util.json_decode(text)
  local decoded, err = minecraft._json_decode(text or "")
  if decoded == nil then
    return nil, err
  end
  return decoded
end

function minecraft.world.marker_px(grid, world_x, world_z)
  if grid == nil or grid.side == nil or grid.step == nil then
    return 0, 0
  end
  local col = math.floor((world_x - (grid.origin_x or 0)) / grid.step + 0.5)
  local row = math.floor((world_z - (grid.origin_z or 0)) / grid.step + 0.5)
  col = minecraft.util.clamp(col, 0, grid.side - 1)
  row = minecraft.util.clamp(row, 0, grid.side - 1)
  return col, row
end

function minecraft.util.parse_boolean(value, fallback)
  if value == nil or value == "" then
    return fallback
  end
  local normalized = minecraft.util.trim(value):lower()
  if normalized == "true" or normalized == "1" or normalized == "yes" or normalized == "on" then
    return true
  end
  if normalized == "false" or normalized == "0" or normalized == "no" or normalized == "off" then
    return false
  end
  return fallback
end

function minecraft.util.copy(values)
  local result = {}
  for key, value in pairs(values or {}) do
    result[key] = value
  end
  return result
end

minecraft.gui.draw_centered_text = function(x, y, width, label, color)
  local text_width = minecraft.gui.text_width(label)
  minecraft.gui.draw_text(x + math.floor((width - text_width) / 2), y, label, color or 0xFFFFFFFF)
end

minecraft.gui.draw_slider = function(x, y, width, height, normalized, label, mouse_x, mouse_y)
  local hover = minecraft.util.in_rect(mouse_x, mouse_y, x, y, width, height)
  minecraft.gui.fill_rect(x, y, width, height, hover and 0xFF808080 or 0xFF606060)
  minecraft.gui.fill_rect(x, y, width, 1, 0xFFAAAAAA)
  minecraft.gui.fill_rect(x, y + height - 1, width, 1, 0xFF303030)
  local knob_x = x + math.floor(minecraft.util.clamp(normalized, 0, 1) * (width - 8))
  minecraft.gui.fill_rect(knob_x, y, 4, height, 0xFFC0C0C0)
  minecraft.gui.fill_rect(knob_x + 4, y, 4, height, 0xFF909090)
  minecraft.gui.draw_centered_text(x + 2, y + 6, width - 4, label)
end

minecraft.gui.draw_toggle = function(x, y, width, height, label, value, mouse_x, mouse_y)
  local hover = minecraft.util.in_rect(mouse_x, mouse_y, x, y, width, height)
  minecraft.gui.fill_rect(x, y, width, height, hover and 0xFF808080 or 0xFF606060)
  minecraft.gui.fill_rect(x + 2, y + 2, 16, height - 4, value and 0xFF40C040 or 0xFFC04040)
  minecraft.gui.draw_text(x + 22, y + 6, label .. ": " .. (value and "ON" or "OFF"), 0xFFFFFFFF)
end

minecraft.config = {}
minecraft.storage = {
  read = native_read_storage,
  write = native_write_storage,
}

function minecraft.config.load(path, defaults, options)
  options = options or {}
  local values = minecraft.util.copy(defaults)
  local text = minecraft.storage.read(path)
  if text == nil or text == "" then
    return values, false
  end
  for line in text:gmatch("[^\r\n]+") do
    local raw_key, raw_value = line:match("^%s*([^#;][^:=]-)%s*[:=]%s*(.-)%s*$")
    if raw_key ~= nil then
      local key = minecraft.util.trim(raw_key)
      key = (options.aliases and options.aliases[key]) or key
      if defaults[key] ~= nil then
        local expected_type = type(defaults[key])
        if expected_type == "boolean" then
          values[key] = minecraft.util.parse_boolean(raw_value, values[key])
        elseif expected_type == "number" then
          values[key] = tonumber(raw_value) or values[key]
        else
          values[key] = raw_value ~= "" and raw_value or values[key]
        end
      end
    end
  end
  return values, true
end

function minecraft.config.save(path, values, options)
  options = options or {}
  local keys = options.keys or {}
  if #keys == 0 then
    for key in pairs(values) do
      keys[#keys + 1] = key
    end
    table.sort(keys)
  end
  local separator = options.separator or "="
  local lines = {}
  for _, key in ipairs(keys) do
    local output_key = (options.names and options.names[key]) or key
    lines[#lines + 1] = output_key .. separator .. tostring(values[key])
  end
  return minecraft.storage.write(path, table.concat(lines, "\n") .. "\n")
end

local module_root = minecraft.asset_path(".")
package.path = module_root .. "/?.lua;" .. module_root .. "/?/init.lua"
local native_require = require
function minecraft.require(module_name)
  assert(type(module_name) == "string", "module name must be a string")
  assert(module_name:match("^[%w_.-]+$") and not module_name:find("..", 1, true),
    "module name must stay inside the mod package")
  return native_require(module_name)
end
require = minecraft.require
package = nil

minecraft._subscribe = nil
minecraft._register_block = nil
minecraft._register_shaped_recipe = nil
minecraft._read_storage = nil
minecraft._write_storage = nil

-- These helpers are Lua compositions over ordinary filtered events. They remain
-- as compatibility aliases while minecraft.on(options, callback) is the core API.
function minecraft.screen.on_ui(screen_id, region, callback, priority)
  return minecraft.on(minecraft.events.screen_ui, {
    screen_id = screen_id,
    region = region,
    priority = priority,
  }, callback)
end

function minecraft.screen.on_lua_screen(screen_id, handlers, priority)
  return minecraft.on(minecraft.events.screen_event, {
    screen_id = screen_id,
    priority = priority,
  }, function(event)
    local handler = handlers[event.phase]
    if handler then
      return handler(event)
    end
  end)
end

-- Declarative two-column settings screen shared by renderer/gameplay mods.
function minecraft.screen.settings(spec)
  local ui = { controls = {}, drag = nil }
  local priority = spec.priority or 100
  local function values()
    return type(spec.values) == "function" and spec.values() or spec.values
  end
  local function layout(width, height)
    local controls = {}
    for _, slider in ipairs(spec.sliders or {}) do
      slider.kind = "slider"
      controls[#controls + 1] = slider
    end
    for _, toggle in ipairs(spec.toggles or {}) do
      toggle.kind = "toggle"
      controls[#controls + 1] = toggle
    end
    local y0 = math.floor(height / 4)
    for index, control in ipairs(controls) do
      control.x = math.floor(width / 2 - 155) + ((index - 1) % 2) * 160
      control.y = y0 + math.floor((index - 1) / 2) * 24
      control.w, control.h = 150, 20
    end
    ui.controls = controls
    return y0 + math.ceil(#controls / 2) * 24 + 24
  end
  local function set_slider(control, mouse_x)
    local normalized = minecraft.util.clamp((mouse_x - control.x - 4) / (control.w - 8), 0, 1)
    local value = control.min + normalized * (control.max - control.min)
    if control.integer then
      value = math.floor(value + 0.5)
    end
    values()[control.key] = value
    apply_change()
  end
  local function close()
    if spec.on_save then
      spec.on_save()
    end
    minecraft.screen.close()
  end
  local function apply_change()
    if spec.on_change then
      spec.on_change()
    end
    if spec.on_save then
      spec.on_save()
    end
  end
  local function open()
    minecraft.screen.open(spec.id)
  end

  minecraft.screen.on_ui(spec.parent_screen, spec.parent_region, function(event)
    if event.ui == nil then
      return event
    end
    event.ui.add_stacked_centered_button(spec.button_label, open)
    return event
  end, priority)

  minecraft.screen.on_lua_screen(spec.id, {
    init = function(event)
      local button_y = layout(event.width, event.height)
      if spec.on_reset then
        minecraft.screen.add_button(math.floor(event.width / 2 - 100), button_y, 200, 20,
          "Reset to Defaults", spec.on_reset)
        button_y = button_y + 24
      end
      minecraft.screen.add_button(math.floor(event.width / 2 - 100), button_y, 200, 20, "Done", close)
    end,
    render = function(event)
      if ui.drag then
        set_slider(ui.drag, event.mouse_x)
      end
      minecraft.gui.draw_centered_text(0, 20, event.width, spec.title)
      local current = values()
      for _, control in ipairs(ui.controls) do
        local value = current[control.key]
        if control.kind == "slider" then
          local normalized = (value - control.min) / (control.max - control.min)
          local label = control.format and control.format(value) or
            ((control.label or control.key) .. ": " .. tostring(value))
          minecraft.gui.draw_slider(control.x, control.y, control.w, control.h, normalized, label,
            event.mouse_x, event.mouse_y)
        else
          minecraft.gui.draw_toggle(control.x, control.y, control.w, control.h, control.label, value,
            event.mouse_x, event.mouse_y)
        end
      end
    end,
    mouse = function(event)
      if event.button ~= 0 then
        return
      end
      if event.released then
        ui.drag = nil
        return
      end
      for i = #ui.controls, 1, -1 do
        local control = ui.controls[i]
        if minecraft.util.in_rect(event.x, event.y, control.x, control.y, control.w, control.h) then
          if control.kind == "slider" then
            ui.drag = control
            set_slider(control, event.x)
          else
            values()[control.key] = not values()[control.key]
            apply_change()
          end
          event.handled = true
          return
        end
      end
    end,
    key = function(event)
      if event.key == minecraft.keys.escape then
        close()
        event.handled = true
      end
    end,
  }, priority)
  return open
end

)lua";
} // namespace net::minecraft::mod::lua
