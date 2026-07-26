#pragma once
#include <string_view>
namespace net::minecraft::mod::lua {
inline constexpr std::string_view kRuntimePrelude = R"lua(
local subscribe = assert(minecraft._subscribe)
local register_block = assert(minecraft._register_block)
local register_item = minecraft._register_item
local register_recipe = assert(minecraft._register_shaped_recipe)
local read_storage = assert(minecraft._read_storage)
local write_storage = assert(minecraft._write_storage)

os = { clock = os.clock, date = os.date, difftime = os.difftime, time = os.time }
io, debug, dofile, loadfile = nil, nil, nil, nil
package.cpath = ""
package.loadlib = nil
package.searchers[3], package.searchers[4] = nil, nil

local function matches(event, options)
  for key, expected in pairs(options) do
    if key ~= "once" and key ~= "priority" and key ~= "when" then
      local actual = event[key]
      if type(expected) == "function" then
        if not expected(actual) then return false end
      elseif type(expected) == "table" then
        local found = expected[actual] == true
        if not found then
          for _, value in ipairs(expected) do
            if value == actual then found = true break end
          end
        end
        if not found then return false end
      elseif actual ~= expected then
        return false
      end
    end
  end
  return options.when == nil or options.when(event) == true
end

function minecraft.on(event_name, options, callback)
  assert(type(options) == "table", "minecraft.on expects (event, options, callback)")
  assert(type(callback) == "function", "minecraft.on expects (event, options, callback)")
  local active = true
  return subscribe(event_name, function(event)
    if active and matches(event, options) then
      local result = callback(event)
      if options.once then active = false end
      return result or event
    end
    return event
  end, tonumber(options.priority) or 0)
end

function minecraft.register_block(spec)
  local ok, err = register_block(spec)
  assert(ok, err)
  if spec.on_use then
    minecraft.on("block_interact", {
      block_id = spec.id, right_click = true, priority = spec.behavior_priority or 0
    }, function(event)
      local result = spec.on_use(event)
      return result
    end)
  end
  return true
end

function minecraft.register_item(spec)
  assert(register_item, "item registration unavailable")
  local ok, err = register_item(spec)
  assert(ok, err)
  return true
end

function minecraft.register_shaped_recipe(spec)
  local ok, err = register_recipe(spec)
  assert(ok, err)
  return true
end

local util = minecraft.util
function util.clamp(value, minimum, maximum) return math.max(minimum, math.min(maximum, value)) end
function util.trim(value) return tostring(value or ""):match("^%s*(.-)%s*$") or "" end
function util.in_rect(x, y, left, top, width, height)
  return x >= left and x < left + width and y >= top and y < top + height
end
function util.real_world(event) return event ~= nil and event.mod_generation ~= false end
function util.parse_boolean(value)
  value = util.trim(value):lower()
  if value == "true" or value == "1" or value == "yes" or value == "on" then return true end
  if value == "false" or value == "0" or value == "no" or value == "off" then return false end
  return nil
end
function util.copy(values)
  local result = {}
  for key, value in pairs(values or {}) do result[key] = value end
  return result
end

minecraft.config = {
  load = function(path, defaults)
    local values = util.copy(defaults)
    local text = read_storage(path)
    if text == nil or text == "" then return values, false end
    for line in text:gmatch("[^\r\n]+") do
      local raw_key, raw_value = line:match("^%s*([^#;][^=]-)%s*=%s*(.-)%s*$")
      if raw_key ~= nil then
        local key = util.trim(raw_key)
        if defaults[key] ~= nil then
          local expected_type = type(defaults[key])
          if expected_type == "boolean" then
            local parsed = util.parse_boolean(raw_value)
            assert(parsed ~= nil, "invalid boolean config value for " .. key)
            values[key] = parsed
          elseif expected_type == "number" then
            local parsed = tonumber(raw_value)
            assert(parsed ~= nil, "invalid numeric config value for " .. key)
            values[key] = parsed
          else
            values[key] = raw_value
          end
        end
      end
    end
    return values, true
  end,
  save = function(path, values, keys)
    keys = keys or {}
    if #keys == 0 then
      for key in pairs(values) do keys[#keys + 1] = key end
      table.sort(keys)
    end
    local lines = {}
    for _, key in ipairs(keys) do
      lines[#lines + 1] = key .. "=" .. tostring(values[key])
    end
    return write_storage(path, table.concat(lines, "\n") .. "\n")
  end,
}

minecraft.storage = { read = read_storage, write = write_storage }
local root_dir = minecraft.asset_path("."):gsub("\\", "/")
local mods_dir = root_dir:match("^(.+)/%.cache/[^/]+$") or root_dir:match("^(.+)/[^/]+$") or root_dir
package.path = root_dir .. "/?.lua;" .. root_dir .. "/?/init.lua;" .. mods_dir .. "/?.lua;" .. mods_dir .. "/?/init.lua;" .. package.path
minecraft._subscribe = nil
minecraft._register_block = nil
minecraft._register_item = nil
minecraft._register_shaped_recipe = nil
minecraft._read_storage = nil
minecraft._write_storage = nil
)lua";
}
