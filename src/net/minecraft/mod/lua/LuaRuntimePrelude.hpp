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

function minecraft.on(event_name, options, callback)
  assert(type(options) == "table", "minecraft.on expects (event, options, callback)")
  assert(type(callback) == "function", "minecraft.on expects (event, options, callback)")
  -- The filter is flattened once here, not walked with pairs() on every dispatch:
  -- entity_render and world_render fire per entity per frame.
  local keys, wanted, count = {}, {}, 0
  for key, value in pairs(options) do
    if key ~= "once" and key ~= "priority" and key ~= "when" then
      count = count + 1
      keys[count], wanted[count] = key, value
    end
  end
  local when, once = options.when, options.once
  local active = true
  return subscribe(event_name, options, function(event)
    if not active then return event end
    for index = 1, count do
      local want = wanted[index]
      local actual = event[keys[index]]
      if actual ~= want then
        local kind = type(want)
        if kind == "function" then
          if not want(actual) then return event end
        elseif kind == "table" then
          if want[actual] ~= true then
            local found = false
            for _, value in ipairs(want) do
              if value == actual then found = true break end
            end
            if not found then return event end
          end
        else
          return event
        end
      end
    end
    if when ~= nil and when(event) ~= true then return event end
    local result = callback(event)
    if once then active = false end
    return result or event
  end, tonumber(options.priority) or 0)
end

local function checked(register, spec)
  local ok, err = register(spec)
  assert(ok, err)
  return true
end

function minecraft.register_block(spec)
  checked(register_block, spec)
  if spec.on_use then
    minecraft.on("block_interact", {
      block_id = spec.id, right_click = true, priority = spec.behavior_priority or 0
    }, spec.on_use)
  end
  return true
end

function minecraft.register_item(spec)
  assert(register_item, "item registration unavailable")
  return checked(register_item, spec)
end

function minecraft.register_shaped_recipe(spec)
  return checked(register_recipe, spec)
end

local util = minecraft.util
function util.clamp(value, minimum, maximum) return math.max(minimum, math.min(maximum, value)) end
function util.in_rect(x, y, left, top, width, height)
  return x >= left and x < left + width and y >= top and y < top + height
end
function util.real_world(event) return event ~= nil and event.mod_generation ~= false end
function util.parse_boolean(value)
  value = tostring(value or ""):match("^%s*(.-)%s*$"):lower()
  if value == "true" or value == "1" or value == "yes" or value == "on" then return true end
  if value == "false" or value == "0" or value == "no" or value == "off" then return false end
  return nil
end

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
