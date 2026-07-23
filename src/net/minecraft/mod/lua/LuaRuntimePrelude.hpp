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

local function names(list)
  local result = {}
  for _, name in ipairs(list) do result[name] = name end
  return result
end

minecraft.events = names({
  "attack_damage", "block_interact", "chunk_generation", "client_tick", "create_world",
  "entity_interact", "entity_remove", "entity_render", "entity_spawn", "entity_teleport", "entity_tick",
  "first_person_hand", "fog_settings", "fov", "camera_setup",
  "key_press", "mouse_button", "player_travel", "pre_entity_render", "pre_tile_entity_render",
  "raycast", "render_frame", "screen_event", "screen_region", "screen_ui",
  "tick_rate", "tile_entity_tick", "world_color", "world_open", "world_render",
  "world_spawn_search", "world_start", "world_tick"
})
minecraft.lifecycle = names({
  "init", "post_init", "ready"
})
minecraft.generation = {
  stages = names({ "terrain", "surface", "carver", "features" }),
  moments = names({ "before", "after" })
}
if minecraft.render ~= nil then
  minecraft.render.stages = names({
    "sky", "stars", "terrain_opaque", "entities", "particles_lit", "particles",
    "terrain_translucent", "weather", "clouds", "hand", "framebuffer"
  })
  minecraft.render.moments = names({ "before", "after" })
end
minecraft.colors = names({ "sky", "fog" })
minecraft.keys = { escape = 1, enter = 28, space = 57, up = 200, down = 208 }

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
  end, tonumber(options.priority) or 0, options)
end

function minecraft.register_block(spec)
  local ok, err = register_block(spec)
  assert(ok, err)
  if spec.on_use then
    minecraft.on(minecraft.events.block_interact, {
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
function util.parse_boolean(value, fallback)
  if value == nil or value == "" then
    minecraft.log("warn", "parse_boolean: using fallback for nil/empty value")
    return fallback
  end
  value = util.trim(value):lower()
  if value == "true" or value == "1" or value == "yes" or value == "on" then return true end
  if value == "false" or value == "0" or value == "no" or value == "off" then return false end
  minecraft.log("warn", "parse_boolean: using fallback for unrecognized value '" .. tostring(value) .. "'")
  return fallback
end
function util.copy(values)
  local result = {}
  for key, value in pairs(values or {}) do result[key] = value end
  return result
end

minecraft.config = {
  load = function(path, defaults, options)
    options = options or {}
    local values = {}
    for k, v in pairs(defaults or {}) do values[k] = v end
    local text = read_storage(path)
    if text == nil or text == "" then return values, false end
    for line in text:gmatch("[^\r\n]+") do
      local raw_key, raw_value = line:match("^%s*([^#;][^:=]-)%s*[:=]%s*(.-)%s*$")
      if raw_key ~= nil then
        local key = util.trim(raw_key)
        key = (options.aliases and options.aliases[key]) or key
        if defaults ~= nil and defaults[key] ~= nil then
          local expected_type = type(defaults[key])
          if expected_type == "boolean" then
            values[key] = util.parse_boolean(raw_value, values[key])
          elseif expected_type == "number" then
            values[key] = tonumber(raw_value) or values[key]
          else
            values[key] = raw_value ~= "" and raw_value or values[key]
          end
        end
      end
    end
    return values, true
  end,
  save = function(path, values, options)
    options = options or {}
    local keys = options.keys or {}
    if #keys == 0 then
      for key in pairs(values or {}) do keys[#keys + 1] = key end
      table.sort(keys)
    end
    local separator = options.separator or "="
    local lines = {}
    for _, key in ipairs(keys) do
      local output_key = (options.names and options.names[key]) or key
      lines[#lines + 1] = output_key .. separator .. tostring(values[key])
    end
    return write_storage(path, table.concat(lines, "\n") .. "\n")
  end,
}

if minecraft.screen ~= nil then
  local screen = minecraft.screen
  function screen.on_ui(screen_id, region, callback, priority)
    return minecraft.on(minecraft.events.screen_ui, {
      screen_id = screen_id, region = region, priority = priority or 0,
    }, callback)
  end
  function screen.on_lua_screen(screen_id, handlers, priority)
    return minecraft.on(minecraft.events.screen_event, {
      screen_id = screen_id, priority = priority or 0,
    }, function(event)
      local handler = handlers[event.phase]
      if handler then return handler(event) or event end
      return event
    end)
  end
  function screen.settings(spec)
    local ui = { controls = {}, drag = nil, page = 1, pages = 1, width = 0, height = 0 }
    local priority = spec.priority or 100
    local parent_region = spec.parent_region or screen.regions.footer
    local function values()
      return type(spec.values) == "function" and spec.values() or spec.values
    end
    local function layout(width, height)
      local all = {}
      for _, slider in ipairs(spec.sliders or {}) do
        slider.kind = "slider"
        all[#all + 1] = slider
      end
      for _, toggle in ipairs(spec.toggles or {}) do
        toggle.kind = "toggle"
        all[#all + 1] = toggle
      end
      ui.width, ui.height = width, height
      local y0 = 44
      local rows = math.max(1, math.floor((height - y0 - 64) / 24))
      local page_size = rows * 2
      ui.pages = math.max(1, math.ceil(#all / page_size))
      ui.page = util.clamp(ui.page, 1, ui.pages)
      local first = (ui.page - 1) * page_size + 1
      local last = math.min(#all, first + page_size - 1)
      local controls = {}
      for index = first, last do
        local control = all[index]
        local visible_index = index - first
        control.x = math.floor(width / 2 - 155) + (visible_index % 2) * 160
        control.y = y0 + math.floor(visible_index / 2) * 24
        control.w, control.h = 150, 20
        controls[#controls + 1] = control
      end
      ui.controls = controls
    end
    local function apply_change()
      if spec.on_change then spec.on_change() end
    end
    local function set_slider(control, mouse_x)
      local normalized = util.clamp((mouse_x - control.x - 4) / (control.w - 8), 0, 1)
      local value = control.min + normalized * (control.max - control.min)
      if control.integer then value = math.floor(value + 0.5) end
      values()[control.key] = value
      apply_change()
    end
    local function close()
      if spec.on_save then spec.on_save() end
      screen.close()
    end
    local function open()
      screen.open(spec.id, { title = spec.title })
    end
    local function change_page(delta)
      ui.page = util.clamp(ui.page + delta, 1, ui.pages)
      ui.drag = nil
      layout(ui.width, ui.height)
    end
    screen.on_ui(spec.parent_screen, parent_region, function(event)
      if event.ui == nil then return event end
      event.ui:add_stacked_centered_button(spec.button_label, open)
      return event
    end, priority)
    screen.on_lua_screen(spec.id, {
      init = function(event)
        ui.page = 1
        layout(event.width, event.height)
        if ui.pages > 1 then
          screen.add_button(math.floor(event.width / 2 - 155), event.height - 52, 150, 20, "< Previous", function()
            change_page(-1)
          end)
          screen.add_button(math.floor(event.width / 2 + 5), event.height - 52, 150, 20, "Next >", function()
            change_page(1)
          end)
        end
        if spec.on_reset then
          screen.add_button(math.floor(event.width / 2 - 155), event.height - 28, 150, 20, "Reset", spec.on_reset)
          screen.add_button(math.floor(event.width / 2 + 5), event.height - 28, 150, 20, "Done", close)
        else
          screen.add_button(math.floor(event.width / 2 - 100), event.height - 28, 200, 20, "Done", close)
        end
      end,
      render = function(event)
        if ui.drag then set_slider(ui.drag, event.mouse_x) end
        if ui.pages > 1 then
          minecraft.gui.draw_centered_text(event.width / 2, 32,
            "Page " .. tostring(ui.page) .. " / " .. tostring(ui.pages), 0xFFA0A0A0)
        end
        local current = values()
        for _, control in ipairs(ui.controls) do
          local value = current[control.key]
          if control.kind == "slider" then
            local normalized = (value - control.min) / (control.max - control.min)
            local label = control.format and control.format(value) or
              ((control.label or control.key) .. ": " .. tostring(value))
            minecraft.gui.draw_slider({ x = control.x, y = control.y, width = control.w, height = control.h,
              value = normalized, text = label, mouse_x = event.mouse_x, mouse_y = event.mouse_y })
          else
            minecraft.gui.draw_toggle({ x = control.x, y = control.y, width = control.w, height = control.h,
              label = control.label, value = value, mouse_x = event.mouse_x, mouse_y = event.mouse_y })
          end
        end
      end,
      mouse = function(event)
        if event.button ~= 0 then return end
        if event.released then ui.drag = nil return end
        for i = #ui.controls, 1, -1 do
          local control = ui.controls[i]
          if util.in_rect(event.x, event.y, control.x, control.y, control.w, control.h) then
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
end

-- Voxel models, built entirely in Lua on top of minecraft.model.build (the
-- engine only bakes arbitrary quads into a handle; all voxel geometry lives
-- here). Each face lists its neighbour offset (for interior-face culling) and
-- its 4 corners as unit-cube selectors (0 = min corner, 1 = max corner).
if minecraft.model ~= nil and minecraft.texture ~= nil then
  local model = minecraft.model
  local texture = minecraft.texture
  local floor = math.floor
  local faces = {
    { n = { 0, 0, 1 },  c = { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} } }, -- +Z
    { n = { 0, 0, -1 }, c = { {1,0,0}, {0,0,0}, {0,1,0}, {1,1,0} } }, -- -Z
    { n = { -1, 0, 0 }, c = { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} } }, -- -X
    { n = { 1, 0, 0 },  c = { {1,0,1}, {1,0,0}, {1,1,0}, {1,1,1} } }, -- +X
    { n = { 0, 1, 0 },  c = { {0,1,1}, {1,1,1}, {1,1,0}, {0,1,0} } }, -- +Y
    { n = { 0, -1, 0 }, c = { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} } }, -- -Y
  }
  local function cell_key(x, y, z) return x .. ":" .. y .. ":" .. z end

  -- model.voxels{cells = {{x,y,z, r,g,b,a}, ...}, resolution = 16,
  --   origin_x/y/z, scale, key} -> handle. Cells are unit cubes on an integer
  -- lattice; faces shared with a present neighbour are culled.
  function model.voxels(opts)
    assert(type(opts) == "table", "model.voxels expects an options table")
    local cells = opts.cells
    assert(type(cells) == "table", "model.voxels requires a cells array")
    local resolution = opts.resolution or 16
    local scale = opts.scale or (resolution > 0 and 1 / resolution or 0)
    assert(scale > 0, "model.voxels resolution/scale must be positive")
    local ox, oy, oz = opts.origin_x or 0, opts.origin_y or 0, opts.origin_z or 0
    local present = {}
    for _, cell in ipairs(cells) do present[cell_key(cell.x, cell.y, cell.z)] = true end
    local quads = {}
    for _, cell in ipairs(cells) do
      local lo = { ox + cell.x * scale, oy + cell.y * scale, oz + cell.z * scale }
      local hi = { lo[1] + scale, lo[2] + scale, lo[3] + scale }
      for _, face in ipairs(faces) do
        if not present[cell_key(cell.x + face.n[1], cell.y + face.n[2], cell.z + face.n[3])] then
          local verts = {}
          for i = 1, 4 do
            local sel = face.c[i]
            verts[i] = {
              x = sel[1] == 1 and hi[1] or lo[1],
              y = sel[2] == 1 and hi[2] or lo[2],
              z = sel[3] == 1 and hi[3] or lo[3],
            }
          end
          quads[#quads + 1] = {
            r = cell.r or 1, g = cell.g or 1, b = cell.b or 1, a = cell.a or 1, vertices = verts,
          }
        end
      end
    end
    if #quads == 0 then return nil, "model.voxels has no cells" end
    return model.build({ quads = quads, key = opts.key })
  end

  -- model.voxel{texture = path, atlas_index?, mod_texture?, grid?} -> handle.
  -- Samples a sprite and extrudes it one voxel thick, centered on z = 0.5.
  local voxel_cache = {}
  function model.voxel(spec)
    assert(type(spec) == "table", "model.voxel expects a spec table")
    local path = spec.texture
    if type(path) ~= "string" or path == "" then
      return nil, "model.voxel requires a texture path"
    end
    local atlas_index = spec.atlas_index or -1
    local mod_texture = spec.mod_texture and true or false
    local grid = spec.grid or 16
    local cutoff = spec.alpha_cutoff or 30
    local cache_key = path .. "|" .. atlas_index .. "|" .. tostring(mod_texture) .. "|" .. grid
    local cached = voxel_cache[cache_key]
    if cached ~= nil then return cached or nil end
    local size = texture.size(path)
    local w, h = size.width or 0, size.height or 0
    if w <= 0 or h <= 0 then
      voxel_cache[cache_key] = false
      return nil, "model.voxel texture not found: " .. path
    end
    local tile_x, tile_y = 0, 0
    if (not mod_texture) and atlas_index >= 0 then
      tile_x = (atlas_index % 16) * 16
      tile_y = floor(atlas_index / 16) * 16
    end
    local cells = {}
    for row = 0, grid - 1 do
      for col = 0, grid - 1 do
        local px, py
        if mod_texture then
          px, py = floor(col * w / grid), floor(row * h / grid)
        else
          px, py = tile_x + col, tile_y + row
        end
        if px >= 0 and py >= 0 and px < w and py < h then
          local p = texture.pixel(path, px, py)
          if p.a > cutoff then
            cells[#cells + 1] = {
              x = col, y = (grid - 1) - row, z = 0,
              r = p.r / 255, g = p.g / 255, b = p.b / 255, a = p.a / 255,
            }
          end
        end
      end
    end
    if #cells == 0 then
      voxel_cache[cache_key] = false
      return nil, "model.voxel sprite has no opaque pixels"
    end
    local handle = model.voxels({
      cells = cells, resolution = grid, origin_z = 0.5 - (1 / grid) / 2, key = "voxel|" .. cache_key,
    })
    voxel_cache[cache_key] = handle or false
    return handle
  end
end

function minecraft.world.marker_px(grid, world_x, world_z)
  if not grid or not grid.side or not grid.step then return 0, 0 end
  local col = math.floor((world_x - (grid.origin_x or 0)) / grid.step + 0.5)
  local row = math.floor((world_z - (grid.origin_z or 0)) / grid.step + 0.5)
  return util.clamp(col, 0, grid.side - 1), util.clamp(row, 0, grid.side - 1)
end

minecraft.storage = { read = read_storage, write = write_storage }
local root_dir = (minecraft.asset_path(".") or ""):gsub("\\", "/")
local mods_dir = root_dir:match("^(.+)/%.cache/[^/]+$") or root_dir:match("^(.+)/[^/]+$") or root_dir
package.path = root_dir .. "/?.lua;" .. root_dir .. "/?/init.lua;" .. mods_dir .. "/?.lua;" .. mods_dir .. "/?/init.lua;" .. package.path
local load_module = require
function minecraft.require(name)
  assert(type(name) == "string" and name:match("^[%w_.-]+$") and not name:find("..", 1, true))
  if minecraft.mod_id ~= nil and name:sub(1, #minecraft.mod_id + 1) == (minecraft.mod_id .. ".") then
    local rel_name = name:sub(#minecraft.mod_id + 2)
    local ok, result = pcall(load_module, rel_name)
    if ok then
      return result
    else
      local not_found_prefix = "module '" .. rel_name .. "' not found:"
      if type(result) == "string" and result:find(not_found_prefix, 1, true) then
      else
        error(result, 0)
      end
    end
  end
  return load_module(name)
end
require = minecraft.require
function minecraft.require_dir(dir)
  assert(type(dir) == "string" and dir:match("^[%w_./-]+$") and not dir:find("..", 1, true))
  local prefix = dir:gsub("/", "."):gsub("^%.+", ""):gsub("%.+$", "")
  local modules = {}
  for _, file in ipairs(minecraft.list_assets(dir) or {}) do
    local stem = file:match("^([%w_-]+)%.lua$")
    if stem ~= nil and stem ~= "init" then
      local ok, result = pcall(minecraft.require, prefix .. "." .. stem)
      if ok and result ~= nil then
        modules[#modules + 1] = { name = stem, module = result }
      elseif not ok then
        minecraft.log("error", "require_dir failed for " .. dir .. "/" .. file .. ": " .. tostring(result))
      else
        minecraft.log("warn", "require_dir: module returned nil (fallback) for " .. dir .. "/" .. file)
      end
    end
  end
  return modules
end
minetest = minecraft
minecraft.event = {}
function minecraft.event.register(name, callback)
  if name == "tick" then
    if minecraft.is_client() then
      return minecraft.on(minecraft.events.client_tick, { after_world = true }, function(event)
        if not event.paused then callback(0.05) end
        return event
      end)
    else
      return minecraft.on(minecraft.events.world_tick, {}, function(event)
        callback(0.05)
        return event
      end)
    end
  elseif name == "render" then
    return minecraft.on(minecraft.events.world_render, { stage = "entities", moment = "after", priority = 120 }, function(event)
      if event.shadow_pass or not event.has_world then return event end
      local camera = {
        x = tonumber(event.camera_x) or 0,
        y = tonumber(event.camera_y) or 0,
        z = tonumber(event.camera_z) or 0,
        yaw = tonumber(event.camera_yaw) or 0,
        pitch = tonumber(event.camera_pitch) or 0,
      }
      callback(camera)
      return event
    end)
  elseif name == "render_clouds" then
    return minecraft.on(minecraft.events.world_render, { stage = "clouds", moment = "before" }, function(event)
      local camera = {
        x = tonumber(event.camera_x) or 0,
        y = tonumber(event.camera_y) or 0,
        z = tonumber(event.camera_z) or 0,
        yaw = tonumber(event.camera_yaw) or 0,
        pitch = tonumber(event.camera_pitch) or 0,
      }
      callback(camera, event.tick_delta or 0)
      return event
    end)
  elseif name == "fov" then
    return minecraft.on(minecraft.events.fov, { priority = 100 }, function(event)
      callback(event)
      return event
    end)
  elseif name == "player_move" then
    return minecraft.on(minecraft.events.player_travel, { is_local_player = true, priority = 100 }, function(event)
      callback(event)
      return event
    end)
  elseif name == "world_unload" or name == "unload" then
    return minecraft.on(minecraft.events.world_open, {}, function(event)
      callback()
      return event
    end)
  else
    local matched_event = minecraft.events[name]
    if matched_event then
      return minecraft.on(matched_event, {}, function(event)
        callback(event)
        return event
      end)
    end
  end
end
minecraft._subscribe = nil
minecraft._register_block = nil
minecraft._register_item = nil
minecraft._register_shaped_recipe = nil
minecraft._read_storage = nil
minecraft._write_storage = nil
)lua";
}
