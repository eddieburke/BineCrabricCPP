-- Celestial Body Rendering Module
-- Handles sun and moon procedural rendering

local config = require("realtime_sky.config.init")
local core = require("lib.core.init")

local CELESTIAL_GRID = config.CELESTIAL_GRID
local SUN_HALF_SIZE = config.SUN_HALF_SIZE
local SKY_BODY_RADIUS = config.SKY_BODY_RADIUS
local TWO_PI = config.TWO_PI
local SUN_TEXTURE_CYCLE_MS = config.SUN_TEXTURE_CYCLE_MS
local SUN_PULSE_CYCLE_MS = config.SUN_PULSE_CYCLE_MS
local SUN_PULSE_AMOUNT = config.SUN_PULSE_AMOUNT
local MOON_MEAN_HALF_SIZE = config.MOON_MEAN_HALF_SIZE

local rendering = {}

-- Moon crater data
local MOON_CRATERS = {
  { 4.7, 4.8, 1.15, -1 },
  { 10.2, 4.2, 0.85, -2 },
  { 8.7, 8.4, 1.35, -1 },
  { 4.3, 10.3, 0.80, -2 },
  { 11.0, 11.0, 1.05, -1 },
  { 6.4, 12.1, 0.55, -2 },
}

local function clamp(value, min_val, max_val)
  if value < min_val then return min_val end
  if value > max_val then return max_val end
  return value
end

local function normalize3(x, y, z)
  local length = math.sqrt(x * x + y * y + z * z)
  if length < 1.0e-8 then return 0.0, 1.0, 0.0 end
  return x / length, y / length, z / length
end

local function cross(ax, ay, az, bx, by, bz)
  return ay * bz - az * by, az * bx - ax * bz, ax * by - ay * bx
end

local function dot3(ax, ay, az, bx, by, bz)
  return ax * bx + ay * by + az * bz
end

-- Calculate body basis vectors
function rendering.body_basis(direction_x, direction_y, direction_z)
  local dx, dy, dz = normalize3(direction_x, direction_y, direction_z)
  local rx, ry, rz
  if math.abs(dy) < 0.98 then
    rx, ry, rz = cross(dx, dy, dz, 0.0, 1.0, 0.0)
  else
    rx, ry, rz = cross(dx, dy, dz, 0.0, 0.0, -1.0)
  end
  rx, ry, rz = normalize3(rx, ry, rz)
  local ux, uy, uz = cross(rx, ry, rz, dx, dy, dz)
  ux, uy, uz = normalize3(ux, uy, uz)
  return dx, dy, dz, rx, ry, rz, ux, uy, uz
end

-- Add pixel quad to packed buffer
function rendering.add_pixel_quad(packed, cursor, rx, ry, rz, ux, uy, uz,
    half_size, grid_x, grid_y, red, green, blue, alpha)
  local pixel = (half_size * 2.0) / CELESTIAL_GRID
  local overlap = pixel * 0.015
  local left = -half_size + grid_x * pixel - overlap
  local right = -half_size + (grid_x + 1) * pixel + overlap
  local top = half_size - grid_y * pixel + overlap
  local bottom = half_size - (grid_y + 1) * pixel - overlap

  local function emit(x_scale, y_scale)
    packed[cursor + 1] = rx * x_scale + ux * y_scale
    packed[cursor + 2] = ry * x_scale + uy * y_scale
    packed[cursor + 3] = rz * x_scale + uz * y_scale
    packed[cursor + 4] = 0.0
    packed[cursor + 5] = 0.0
    packed[cursor + 6] = red
    packed[cursor + 7] = green
    packed[cursor + 8] = blue
    packed[cursor + 9] = alpha
    cursor = cursor + 9
  end

  cursor = emit(left, bottom)
  cursor = emit(right, bottom)
  cursor = emit(right, top)
  cursor = emit(left, top)
  return cursor
end

-- Draw pixel vertices
function rendering.draw_pixel_vertices(event, dx, dy, dz, packed, count)
  if count < 4 then return 0 end
  for index = #packed, count * 9 + 1, -1 do
    packed[index] = nil
  end
  return minecraft.render.quads({
    x = (event.camera_x or 0.0) + dx * SKY_BODY_RADIUS,
    y = (event.camera_y or 0.0) + dy * SKY_BODY_RADIUS,
    z = (event.camera_z or 0.0) + dz * SKY_BODY_RADIUS,
    world_space = true,
    blend = true,
    cull = false,
    depth_test = false,
    depth_write = false,
    packed = packed,
  })
end

-- Sun pixel color calculation
function rendering.sun_pixel_color(x, y, animation_phase)
  local edge = math.min(x, CELESTIAL_GRID - 1 - x, y, CELESTIAL_GRID - 1 - y)

  local red, green, blue
  if edge == 0 then
    red, green, blue = 1.000, 0.925, 0.610
  elseif edge == 1 then
    red, green, blue = 1.000, 0.958, 0.720
  else
    red, green, blue = 1.000, 0.978, 0.800
  end

  local light_x = 7.5 + math.cos(animation_phase) * 2.7
  local light_y = 7.5 + math.sin(animation_phase * 0.82) * 2.1
  local warm_x = 15.0 - light_x
  local warm_y = 15.0 - light_y

  local dx_light = (x + 0.5) - light_x
  local dy_light = (y + 0.5) - light_y
  local dx_warm = (x + 0.5) - warm_x
  local dy_warm = (y + 0.5) - warm_y

  local light_weight = math.max(0.0,
    1.0 - (dx_light * dx_light + dy_light * dy_light) / 22.0)
  local warm_weight = math.max(0.0,
    1.0 - (dx_warm * dx_warm + dy_warm * dy_warm) / 25.0)

  local interior = edge >= 2 and 1.0 or (edge == 1 and 0.35 or 0.0)
  green = green + light_weight * 0.010 * interior - warm_weight * 0.012 * interior
  blue = blue + light_weight * 0.018 * interior - warm_weight * 0.020 * interior

  return clamp(red, 0.0, 1.0), clamp(green, 0.0, 1.0), clamp(blue, 0.0, 1.0)
end

-- Draw procedural sun
local celestial_packed = {}

function rendering.draw_procedural_sun(event, frame, alpha)
  if alpha <= 0.001 then return end

  local millis = tonumber(frame.utc_millis) or 0.0
  local texture_phase = (millis % SUN_TEXTURE_CYCLE_MS) / SUN_TEXTURE_CYCLE_MS * TWO_PI
  local pulse_phase = (millis % SUN_PULSE_CYCLE_MS) / SUN_PULSE_CYCLE_MS * TWO_PI

  local animated_half_size = SUN_HALF_SIZE * (1.0 + math.sin(pulse_phase) * SUN_PULSE_AMOUNT)
  local brightness = 0.992 + 0.008 * math.sin(pulse_phase + 0.7)

  local dx, dy, dz, rx, ry, rz, ux, uy, uz = rendering.body_basis(
    frame.sun_direction_x, frame.sun_direction_y, frame.sun_direction_z)
  
  local packed = celestial_packed
  local cursor = 0

  for y = 0, CELESTIAL_GRID - 1 do
    for x = 0, CELESTIAL_GRID - 1 do
      local red, green, blue = rendering.sun_pixel_color(x, y, texture_phase)
      cursor = rendering.add_pixel_quad(packed, cursor, rx, ry, rz, ux, uy, uz, 
        animated_half_size, x, y,
        clamp(red * brightness, 0.0, 1.0),
        clamp(green * brightness, 0.0, 1.0),
        clamp(blue * brightness, 0.0, 1.0),
        alpha)
    end
  end

  rendering.draw_pixel_vertices(event, dx, dy, dz, packed, cursor)
end

-- Moon pixel color
function rendering.moon_pixel_color(x, y, nx, ny, light_dot, near_limb)
  local surface = 0.60 + 0.18 * (-nx) + 0.13 * ny + 0.22 * math.max(light_dot, 0.0)
  surface = surface + ((((x * 5 + y * 3) % 4) - 1.5) * 0.018)

  local crater_delta = 0
  for _, crater in ipairs(MOON_CRATERS) do
    local cx, cy, radius, amount = crater[1], crater[2], crater[3], crater[4]
    local px = (x + 0.5) - cx
    local py = (y + 0.5) - cy
    if px * px + py * py <= radius * radius then
      crater_delta = math.min(crater_delta, amount)
    end
  end
  surface = surface + crater_delta * 0.075

  if near_limb then return 0.47, 0.54, 0.60 end
  if surface < 0.43 then return 0.40, 0.47, 0.54 end
  if surface < 0.53 then return 0.52, 0.58, 0.63 end
  if surface < 0.64 then return 0.59, 0.64, 0.69 end
  if surface < 0.76 then return 0.76, 0.80, 0.84 end
  if surface < 0.87 then return 0.87, 0.89, 0.91 end
  return 0.94, 0.95, 0.96
end

return rendering
