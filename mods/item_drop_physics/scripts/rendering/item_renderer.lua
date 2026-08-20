local engine = require("scripts.physics.engine")
local config = require("config")

local M = {}
local abs, floor, min, max = math.abs, math.floor, math.min, math.max

local DRAW_SCALE = config.draw_scale
local GRID = 16
local ICON_DEPTH = DRAW_SCALE / GRID
local MIN_HALF = DRAW_SCALE / GRID * 0.75

local voxel_handles = {}
local profile_cache = {}
local shape_cache = {}

local VOXEL_FACES = {
  { 0, 0, 1, { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} } },
  { 0, 0, -1, { {1,0,0}, {0,0,0}, {0,1,0}, {1,1,0} } },
  { -1, 0, 0, { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} } },
  { 1, 0, 0, { {1,0,1}, {1,0,0}, {1,1,0}, {1,1,1} } },
  { 0, 1, 0, { {0,1,1}, {1,1,1}, {1,1,0}, {0,1,0} } },
  { 0, -1, 0, { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} } },
}

local function item_key(item)
  return table.concat({
    item.item_id or 0,
    item.item_damage or 0,
    item.texture_path or "",
    item.atlas_index or -1,
    item.mod_texture and 1 or 0,
  }, ":")
end

local function texture_key(item)
  return table.concat({
    item.texture_path or "",
    item.atlas_index or -1,
    item.mod_texture and 1 or 0,
  }, ":")
end

local function voxel_key(x, y, z)
  return x .. ":" .. y .. ":" .. z
end

local function load_profile(item)
  local path = item.texture_path
  if not path or path == "" then return nil end

  local key = texture_key(item)
  local cached = profile_cache[key]
  if cached ~= nil then return cached or nil end

  local ok, size = pcall(minecraft.texture.size, path)
  if not ok or not size or not size.width or not size.height or size.width <= 0 or size.height <= 0 then
    profile_cache[key] = false
    return nil
  end

  local width, height = size.width, size.height
  local atlas_index = item.atlas_index or -1
  local tile_width, tile_height = width, height
  local tile_x, tile_y = 0, 0
  if not item.mod_texture and atlas_index >= 0 then
    tile_width = floor(width / GRID)
    tile_height = floor(height / GRID)
    if tile_width < 1 or tile_height < 1 then
      profile_cache[key] = false
      return nil
    end
    tile_x = (atlas_index % GRID) * tile_width
    tile_y = floor(atlas_index / GRID) * tile_height
  end

  local cells, present = {}, {}
  local mass, sum_x, sum_y, sum_x2, sum_y2 = 0.0, 0.0, 0.0, 0.0, 0.0
  local min_x, min_y, max_x, max_y = GRID, GRID, -1, -1

  for row = 0, GRID - 1 do
    for col = 0, GRID - 1 do
      local px = tile_x + min(tile_width - 1, floor((col + 0.5) * tile_width / GRID))
      local py = tile_y + min(tile_height - 1, floor((row + 0.5) * tile_height / GRID))
      local pixel_ok, pixel = pcall(minecraft.texture.pixel, path, px, py)
      if pixel_ok and pixel and pixel.a and pixel.a > 30 then
        local y = GRID - 1 - row
        local weight = pixel.a / 255.0
        local cx, cy = (col + 0.5) / GRID, (y + 0.5) / GRID
        local cell = { x = col, y = y, z = 0, px = px, py = py, a = pixel.a / 255.0 }
        cells[#cells + 1] = cell
        present[voxel_key(col, y, 0)] = true
        mass = mass + weight
        sum_x = sum_x + cx * weight
        sum_y = sum_y + cy * weight
        sum_x2 = sum_x2 + cx * cx * weight
        sum_y2 = sum_y2 + cy * cy * weight
        min_x, min_y = min(min_x, col), min(min_y, y)
        max_x, max_y = max(max_x, col), max(max_y, y)
      end
    end
  end

  if mass <= 0.0 then
    profile_cache[key] = false
    return nil
  end

  local cx, cy = sum_x / mass, sum_y / mass
  local profile = {
    key = key,
    path = path,
    width = width,
    height = height,
    cells = cells,
    present = present,
    mass = mass,
    cx = cx,
    cy = cy,
    variance_x = max(0.0, sum_x2 / mass - cx * cx),
    variance_y = max(0.0, sum_y2 / mass - cy * cy),
    min_x = min_x / GRID,
    min_y = min_y / GRID,
    max_x = (max_x + 1) / GRID,
    max_y = (max_y + 1) / GRID,
  }
  profile_cache[key] = profile
  return profile
end

local function texture_shape(profile)
  local half_x = max(MIN_HALF, max(profile.cx - profile.min_x, profile.max_x - profile.cx) * DRAW_SCALE)
  local half_y = max(MIN_HALF, max(profile.cy - profile.min_y, profile.max_y - profile.cy) * DRAW_SCALE)
  local half_z = ICON_DEPTH * 0.5
  local cell_variance = 1.0 / (12.0 * GRID * GRID)
  local depth_variance = ICON_DEPTH * ICON_DEPTH / 12.0
  local scale_sq = DRAW_SCALE * DRAW_SCALE
  local inertia_x = (profile.variance_y + cell_variance) * scale_sq + depth_variance
  local inertia_y = (profile.variance_x + cell_variance) * scale_sq + depth_variance
  local inertia_z = (profile.variance_x + profile.variance_y + cell_variance * 2.0) * scale_sq
  local cell_size = DRAW_SCALE / GRID
  return {
    hx = half_x,
    hy = half_y,
    hz = half_z,
    center_x = (profile.cx - 0.5) * DRAW_SCALE,
    center_y = (profile.cy - 0.5) * DRAW_SCALE,
    center_z = 0.0,
    inv_ix = 1.0 / max(inertia_x, 1e-5),
    inv_iy = 1.0 / max(inertia_y, 1e-5),
    inv_iz = 1.0 / max(inertia_z, 1e-5),
    volume = max(cell_size * cell_size * ICON_DEPTH, profile.mass * cell_size * cell_size * ICON_DEPTH),
    blocky = false,
  }
end

local function bounds_shape(bounds)
  local size_x = max(0.02, min(0.60, (bounds.max_x - bounds.min_x) * DRAW_SCALE))
  local size_y = max(0.02, min(0.60, (bounds.max_y - bounds.min_y) * DRAW_SCALE))
  local size_z = max(0.012, min(0.60, (bounds.max_z - bounds.min_z) * DRAW_SCALE))
  local hx, hy, hz = size_x * 0.5, size_y * 0.5, size_z * 0.5
  local largest = max(hx, hy, hz)
  local smallest = min(hx, hy, hz)
  return {
    hx = hx,
    hy = hy,
    hz = hz,
    center_x = ((bounds.min_x + bounds.max_x) * 0.5 - 0.5) * DRAW_SCALE,
    center_y = ((bounds.min_y + bounds.max_y) * 0.5 - 0.5) * DRAW_SCALE,
    center_z = ((bounds.min_z + bounds.max_z) * 0.5 - 0.5) * DRAW_SCALE,
    inv_ix = 3.0 / max(hy * hy + hz * hz, 1e-5),
    inv_iy = 3.0 / max(hx * hx + hz * hz, 1e-5),
    inv_iz = 3.0 / max(hx * hx + hy * hy, 1e-5),
    volume = size_x * size_y * size_z,
    blocky = smallest / largest > 0.72,
  }
end

local function sprite_bounds_shape(bounds)
  local size_x = max(0.02, min(0.60, (bounds.max_x - bounds.min_x) * DRAW_SCALE))
  local size_y = max(0.02, min(0.60, (bounds.max_y - bounds.min_y) * DRAW_SCALE))
  local size_z = ICON_DEPTH
  local hx, hy, hz = size_x * 0.5, size_y * 0.5, size_z * 0.5
  return {
    hx = hx,
    hy = hy,
    hz = hz,
    center_x = ((bounds.min_x + bounds.max_x) * 0.5 - 0.5) * DRAW_SCALE,
    center_y = ((bounds.min_y + bounds.max_y) * 0.5 - 0.5) * DRAW_SCALE,
    center_z = 0.0,
    inv_ix = 3.0 / max(hy * hy + hz * hz, 1e-5),
    inv_iy = 3.0 / max(hx * hx + hz * hz, 1e-5),
    inv_iz = 3.0 / max(hx * hx + hy * hy, 1e-5),
    volume = size_x * size_y * size_z,
    blocky = false,
  }
end

function M.physics_shape(item)
  local key = item_key(item)
  local shape = shape_cache[key]
  if shape then return shape end

  local bounds_ok, bounds = pcall(minecraft.model.item_bounds, item.item_id, item.item_damage or 0)
  if not bounds_ok then bounds = nil end
  local profile = load_profile(item)
  local flat = not bounds or (bounds.max_z - bounds.min_z) <= min(bounds.max_x - bounds.min_x, bounds.max_y - bounds.min_y) * 0.25
  if flat then
    shape = profile and texture_shape(profile) or (bounds and sprite_bounds_shape(bounds))
  elseif bounds then
    shape = bounds_shape(bounds)
  end
  if not shape then
    shape = bounds_shape({ min_x = 0, min_y = 0, min_z = 0.46875, max_x = 1, max_y = 1, max_z = 0.53125 })
  end

  shape_cache[key] = shape
  return shape
end


function M.half_extents(item)
  local shape = M.physics_shape(item)
  return shape.hx, shape.hy, shape.hz
end

local function build_voxel_handle(item)
  local profile = load_profile(item)
  if not profile then return nil end

  local scale = 1.0 / GRID
  local quads = {}
  for _, cell in ipairs(profile.cells) do
    local lo = { cell.x * scale, cell.y * scale, 0.5 - scale * 0.5 }
    local hi = { lo[1] + scale, lo[2] + scale, lo[3] + scale }
    local u, v = (cell.px + 0.5) / profile.width, (cell.py + 0.5) / profile.height
    for _, face in ipairs(VOXEL_FACES) do
      if not profile.present[voxel_key(cell.x + face[1], cell.y + face[2], cell.z + face[3])] then
        local vertices = {}
        for index, selector in ipairs(face[4]) do
          vertices[index] = {
            x = selector[1] == 1 and hi[1] or lo[1],
            y = selector[2] == 1 and hi[2] or lo[2],
            z = selector[3] == 1 and hi[3] or lo[3],
            u = u,
            v = v,
          }
        end
        quads[#quads + 1] = { texture = profile.path, a = cell.a, vertices = vertices }
      end
    end
  end
  return minecraft.model.build({ quads = quads, key = "item-voxel|" .. profile.key })
end

function M.voxel_handle(item)
  local key = texture_key(item)
  local handle = voxel_handles[key]
  if handle == nil then
    local ok, result = pcall(build_voxel_handle, item)
    handle = ok and result or false
    if not handle then handle = false end
    voxel_handles[key] = handle
  end
  return handle or nil
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

local function rotated_center(s, qx, qy, qz, qw)
  local ox, oy, oz = s.center_x, s.center_y, s.center_z
  local xx, yy, zz = qx * qx, qy * qy, qz * qz
  local xy, xz, yz = qx * qy, qx * qz, qy * qz
  local xw, yw, zw = qx * qw, qy * qw, qz * qw
  return
    (1.0 - 2.0 * (yy + zz)) * ox + 2.0 * (xy - zw) * oy + 2.0 * (xz + yw) * oz,
    2.0 * (xy + zw) * ox + (1.0 - 2.0 * (xx + zz)) * oy + 2.0 * (yz - xw) * oz,
    2.0 * (xz - yw) * ox + 2.0 * (yz + xw) * oy + (1.0 - 2.0 * (xx + yy)) * oz
end

function M.draw(item, s, delta)
  local qx, qy, qz, qw =
    engine.quat_slerp(s.pqx, s.pqy, s.pqz, s.pqw, s.qx, s.qy, s.qz, s.qw, delta)
  local center_x, center_y, center_z = rotated_center(s, qx, qy, qz, qw)
  local x = s.px + (s.x - s.px) * delta - center_x
  local y = s.py + (s.y - s.py) * delta - center_y
  local z = s.pz + (s.z - s.pz) * delta - center_z
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
