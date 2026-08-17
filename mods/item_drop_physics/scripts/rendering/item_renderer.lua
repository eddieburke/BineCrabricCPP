-- Builds the voxel fallback model for flat item icons and draws simulated
-- items. Physics state arrives as flat scalars from scripts/physics/engine.lua;
-- this module owns no simulation state of its own.

local engine = require("scripts.physics.engine")
local config = require("config")

local M = {}
local floor, min, max = math.floor, math.min, math.max

local DRAW_SCALE = config.draw_scale
local ICON_THICKNESS = DRAW_SCALE / 16.0
local DEFAULT_HALF = 0.125
local DEFAULT_HEIGHT = 0.25
local GRID = 16

local voxel_handles = {}
local shape_cache = {}

-- Face normal plus the corner selectors for that face's quad.
local VOXEL_FACES = {
  { 0, 0, 1, { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} } },
  { 0, 0, -1, { {1,0,0}, {0,0,0}, {0,1,0}, {1,1,0} } },
  { -1, 0, 0, { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} } },
  { 1, 0, 0, { {1,0,1}, {1,0,0}, {1,1,0}, {1,1,1} } },
  { 0, 1, 0, { {0,1,1}, {1,1,1}, {1,1,0}, {0,1,0} } },
  { 0, -1, 0, { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} } },
}

local function voxel_key(x, y, z)
  return x .. ":" .. y .. ":" .. z
end

local function build_voxel_handle(item)
  local path = item.texture_path
  local size = minecraft.texture.size(path)
  if not size then return nil end
  local width, height = size.width, size.height
  if width <= 0 or height <= 0 then return nil end

  local atlas_index = item.atlas_index or -1
  local tile_x, tile_y = 0, 0
  if not item.mod_texture and atlas_index >= 0 then
    tile_x = (atlas_index % GRID) * GRID
    tile_y = math.floor(atlas_index / GRID) * GRID
  end

  local cells, present = {}, {}
  for row = 0, GRID - 1 do
    for col = 0, GRID - 1 do
      local px, py
      if item.mod_texture then
        px = math.floor(col * width / GRID)
        py = math.floor(row * height / GRID)
      else
        px, py = tile_x + col, tile_y + row
      end
      if px >= 0 and py >= 0 and px < width and py < height then
        local pixel = minecraft.texture.pixel(path, px, py)
        if pixel and pixel.a and pixel.a > 30 then
          local cell = {
            x = col, y = GRID - 1 - row, z = 0,
            r = pixel.r / 255, g = pixel.g / 255, b = pixel.b / 255, a = pixel.a / 255,
          }
          cells[#cells + 1] = cell
          present[voxel_key(cell.x, cell.y, cell.z)] = true
        end
      end
    end
  end
  if #cells == 0 then return nil end

  local scale = 1 / GRID
  local quads = {}
  for _, cell in ipairs(cells) do
    local lo = { cell.x * scale, cell.y * scale, 0.5 - scale * 0.5 }
    local hi = { lo[1] + scale, lo[2] + scale, lo[3] + scale }
    for _, face in ipairs(VOXEL_FACES) do
      -- Skip faces buried against a neighbouring opaque texel.
      if not present[voxel_key(cell.x + face[1], cell.y + face[2], cell.z + face[3])] then
        local vertices = {}
        for index, selector in ipairs(face[4]) do
          vertices[index] = {
            x = selector[1] == 1 and hi[1] or lo[1],
            y = selector[2] == 1 and hi[2] or lo[2],
            z = selector[3] == 1 and hi[3] or lo[3],
          }
        end
        quads[#quads + 1] = { r = cell.r, g = cell.g, b = cell.b, a = cell.a, vertices = vertices }
      end
    end
  end
  return minecraft.model.build({ quads = quads, key = "item-voxel|" .. path .. "|" .. atlas_index })
end

function M.voxel_handle(item)
  local path = item.texture_path
  if not path or path == "" then return nil end

  local key = path .. ":" .. (item.atlas_index or -1) .. ":" .. (item.mod_texture and 1 or 0)
  local handle = voxel_handles[key]
  if handle == nil then
    local ok, result = pcall(build_voxel_handle, item)
    handle = ok and result or false
    if not handle then handle = false end
    voxel_handles[key] = handle
  end
  return handle or nil
end

function M.half_extents(item)
  local key = item.item_id .. ":" .. (item.item_damage or 0)
  local shape = shape_cache[key]

  if shape == nil then
    local bounds = minecraft.model.item_bounds(item.item_id, item.item_damage or 0)
    if bounds then
      shape = {
        max(0.035, min(0.30, (bounds.max_x - bounds.min_x) * 0.5 * DRAW_SCALE)),
        max(0.035, min(0.30, (bounds.max_y - bounds.min_y) * 0.5 * DRAW_SCALE)),
        max(0.015, min(0.30, (bounds.max_z - bounds.min_z) * 0.5 * DRAW_SCALE)),
      }
    else
      shape = { DEFAULT_HALF, DEFAULT_HEIGHT * 0.5, ICON_THICKNESS * 0.5 }
    end
    shape_cache[key] = shape
  end

  return shape[1], shape[2], shape[3]
end

local transform = {
  x = 0, y = 0, z = 0, yaw = 0, pitch = 0, roll = 0,
  pivot_y = 0.5, scale = DRAW_SCALE, entity_id = 0, shader_entity = "item",
}

local function noise(seed)
  local value = math.sin(seed * 12.9898) * 43758.5453
  return (value - floor(value)) * 2.0 - 1.0
end

local function copies(count)
  if count > 20 then return 4 end
  if count > 5 then return 3 end
  if count > 1 then return 2 end
  return 1
end

local function draw_current(item)
  if minecraft.model.draw_item(item.item_id, item.item_damage or 0, transform) then return true end
  local handle = M.voxel_handle(item)
  return handle ~= nil and minecraft.model.draw(handle, transform) or false
end

function M.draw(item, s, delta)
  local qx, qy, qz, qw =
    engine.quat_slerp(s.pqx, s.pqy, s.pqz, s.pqw, s.qx, s.qy, s.qz, s.qw, delta)

  local x = s.px + (s.x - s.px) * delta
  local y = s.py + (s.y - s.py) * delta
  local z = s.pz + (s.z - s.pz) * delta
  local yaw, pitch, roll = engine.quat_to_euler_degrees(qx, qy, qz, qw)
  local count = copies(item.item_count or s.item_count or 1)
  local spread = DRAW_SCALE * 0.24
  local drawn = false
  transform.entity_id = item.id or 0

  for i = 1, count do
    if i == 1 then
      transform.x, transform.y, transform.z = x, y, z
      transform.yaw, transform.pitch, transform.roll = yaw, pitch, roll
    else
      local seed = (item.id or 0) * 0.754877666 + i * 17.0
      transform.x = x + noise(seed) * spread
      transform.y = y + (noise(seed + 2.0) * 0.25 + 0.55) * spread
      transform.z = z + noise(seed + 4.0) * spread
      transform.yaw = yaw + noise(seed + 6.0) * 14.0
      transform.pitch = pitch + noise(seed + 8.0) * 10.0
      transform.roll = roll + noise(seed + 10.0) * 10.0
    end
    if draw_current(item) then drawn = true end
  end
  return drawn
end

return M
