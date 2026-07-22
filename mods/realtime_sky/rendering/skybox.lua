-- Skybox Rendering Module
-- Handles sky dome and celestial body rendering

local config = require("realtime_sky.config.init")
local core = require("lib.core.init")
local math_util = require("lib.math_util")

local SKY_DOME_RADIUS = config.SKY_DOME_RADIUS
local CELESTIAL_GRID = config.CELESTIAL_GRID
local SUN_HALF_SIZE = config.SUN_HALF_SIZE
local TWO_PI = config.TWO_PI

local rendering = {}

-- Cached sky dome directions
local sky_dome_directions = nil

local function clamp(value, min_val, max_val)
  if value < min_val then return min_val end
  if value > max_val then return max_val end
  return value
end

local function smoothstep(edge0, edge1, value)
  local t = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)
end

local function normalize3(x, y, z)
  local length = math.sqrt(x * x + y * y + z * z)
  if length < 1.0e-8 then return 0.0, 1.0, 0.0 end
  return x / length, y / length, z / length
end

local function cross(ax, ay, az, bx, by, bz)
  return ay * bz - az * by,
         az * bx - ax * bz,
         ax * by - ay * bx
end

-- Calculate sky color based on sun altitude
local function sky_color_for_frame(frame)
  local alt = frame.sun_altitude_deg
  local day = smoothstep(-6.0, 12.0, alt)
  local warm = smoothstep(-12.0, -1.0, alt) * (1.0 - smoothstep(1.0, 14.0, alt))
  return 0.015 + (0.45 - 0.015) * day + 0.28 * warm,
         0.025 + (0.65 - 0.025) * day + 0.08 * warm,
         0.070 + (1.00 - 0.070) * day + 0.02 * warm
end

-- Calculate vertex color for sky dome
function rendering.sky_vertex_color(frame, x, y, z)
  local zenith_r, zenith_g, zenith_b = sky_color_for_frame(frame)
  local day = smoothstep(-8.0, 10.0, frame.sun_altitude_deg)
  local twilight = smoothstep(-14.0, -1.0, frame.sun_altitude_deg) *
                   (1.0 - smoothstep(1.0, 12.0, frame.sun_altitude_deg))

  local horizon_r = 0.025 + 0.66 * day + 0.28 * twilight
  local horizon_g = 0.035 + 0.77 * day + 0.11 * twilight
  local horizon_b = 0.085 + 0.91 * day + 0.02 * twilight
  local below_r = 0.010 + 0.12 * day
  local below_g = 0.015 + 0.18 * day
  local below_b = 0.035 + 0.27 * day

  local r, g, b
  if y >= 0.0 then
    local t = smoothstep(0.0, 0.88, y)
    r = horizon_r + (zenith_r - horizon_r) * t
    g = horizon_g + (zenith_g - horizon_g) * t
    b = horizon_b + (zenith_b - horizon_b) * t
  else
    local t = smoothstep(-1.0, 0.0, y)
    r = below_r + (horizon_r - below_r) * t
    g = below_g + (horizon_g - below_g) * t
    b = below_b + (horizon_b - below_b) * t
  end

  -- Sunrise/sunset glow
  if twilight > 0.0 then
    local horizontal_len = math.sqrt(x * x + z * z)
    local sun_horizontal_len = math.sqrt(
      frame.sun_direction_x * frame.sun_direction_x +
      frame.sun_direction_z * frame.sun_direction_z)
    if horizontal_len > 1.0e-6 and sun_horizontal_len > 1.0e-6 then
      local alignment = (x * frame.sun_direction_x + z * frame.sun_direction_z) /
                        (horizontal_len * sun_horizontal_len)
      local glow = smoothstep(0.72, 0.995, alignment) *
                   (1.0 - smoothstep(0.10, 0.55, math.abs(y))) * twilight
      r = r + 0.34 * glow
      g = g + 0.12 * glow
      b = b + 0.015 * glow
    end
  end

  return clamp(r, 0.0, 1.0), clamp(g, 0.0, 1.0), clamp(b, 0.0, 1.0)
end

-- Build sky dome geometry
local function build_sky_dome_directions()
  if sky_dome_directions ~= nil then return sky_dome_directions end
  
  local vertices = {}
  local latitude_steps = 12
  local longitude_steps = 32
  
  for lat_index = 0, latitude_steps - 1 do
    local lat0 = -math.pi * 0.5 + math.pi * lat_index / latitude_steps
    local lat1 = -math.pi * 0.5 + math.pi * (lat_index + 1) / latitude_steps
    local c0, s0 = math.cos(lat0), math.sin(lat0)
    local c1, s1 = math.cos(lat1), math.sin(lat1)
    
    for lon_index = 0, longitude_steps - 1 do
      local lon0 = math.pi * 2.0 * lon_index / longitude_steps
      local lon1 = math.pi * 2.0 * (lon_index + 1) / longitude_steps
      
      local function direction(c, s, lon)
        return { x = c * math.sin(lon), y = s, z = -c * math.cos(lon) }
      end
      
      vertices[#vertices + 1] = direction(c0, s0, lon0)
      vertices[#vertices + 1] = direction(c0, s0, lon1)
      vertices[#vertices + 1] = direction(c1, s1, lon1)
      vertices[#vertices + 1] = direction(c1, s1, lon0)
    end
  end
  
  sky_dome_directions = vertices
  return vertices
end

-- Render sky dome
local sky_dome_packed = {}

function rendering.draw_sky_dome(event, frame)
  local directions = build_sky_dome_directions()
  local packed = sky_dome_packed
  local cursor = 0
  
  for index = 1, #directions do
    local direction = directions[index]
    local dx, dy, dz = direction.x, direction.y, direction.z
    local r, g, b = rendering.sky_vertex_color(frame, dx, dy, dz)
    
    packed[cursor + 1] = dx * SKY_DOME_RADIUS
    packed[cursor + 2] = dy * SKY_DOME_RADIUS
    packed[cursor + 3] = dz * SKY_DOME_RADIUS
    packed[cursor + 4] = 0.0
    packed[cursor + 5] = 0.0
    packed[cursor + 6] = r
    packed[cursor + 7] = g
    packed[cursor + 8] = b
    packed[cursor + 9] = 1.0
    cursor = cursor + 9
  end
  
  for index = #packed, cursor + 1, -1 do
    packed[index] = nil
  end
  
  minecraft.render.quads({
    x = event.camera_x or 0.0,
    y = event.camera_y or 0.0,
    z = event.camera_z or 0.0,
    world_space = true,
    blend = false,
    cull = false,
    depth_test = false,
    depth_write = false,
    packed = packed,
  })
end

return rendering
