local clamp = minecraft.util.clamp

local sky_render = {}

local function smoothstep(edge0, edge1, value)
  local t = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)
end

local SKY_DOME_RADIUS = 80.0
local SKY_BODY_RADIUS = 75.0
local CELESTIAL_GRID = 16
local SUN_HALF_SIZE = 3.65
local SUN_TEXTURE_CYCLE_MS = 18000.0
local SUN_PULSE_CYCLE_MS = 8000.0
local SUN_PULSE_AMOUNT = 0.010
local MOON_MEAN_HALF_SIZE = 2.00
local TWO_PI = math.pi * 2.0

local STAR_COUNT = 200
local STAR_SPHERE_RADIUS = 80.0
local STAR_HALF_SIZE = 0.12

sky_render.MOON_MEAN_HALF_SIZE = MOON_MEAN_HALF_SIZE

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

local function dot3(ax, ay, az, bx, by, bz)
  return ax * bx + ay * by + az * bz
end

local function hash01(seed)
  local n = (seed * 1664525 + 1013904223) % 4294967296
  return (n % 10000) / 10000.0
end

function sky_render.sky_rgb(frame)
  local alt = frame.sun_altitude_deg
  local day = smoothstep(-6.0, 12.0, alt)
  local warm = smoothstep(-12.0, -1.0, alt) * (1.0 - smoothstep(1.0, 14.0, alt))
  return 0.015 + (0.45 - 0.015) * day + 0.12 * warm,
    0.025 + (0.65 - 0.025) * day + 0.05 * warm,
    0.070 + (1.00 - 0.070) * day + 0.02 * warm
end

function sky_render.fog_rgb(frame)
  local alt = frame.sun_altitude_deg
  local day = smoothstep(-6.0, 10.0, alt)
  local warm = smoothstep(-12.0, -1.0, alt) * (1.0 - smoothstep(1.0, 12.0, alt))
  return 0.025 + (0.75 - 0.025) * day + 0.10 * warm,
    0.035 + (0.85 - 0.035) * day + 0.04 * warm,
    0.080 + (1.00 - 0.080) * day
end

function sky_render.star_brightness(frame)
  local darkness = 1.0 - smoothstep(-18.0, -4.0, frame.sun_altitude_deg)
  return darkness * darkness * 0.5
end

local function sky_vertex_color(frame, x, y, z)
  local zenith_r, zenith_g, zenith_b = sky_render.sky_rgb(frame)
  local day = smoothstep(-8.0, 10.0, frame.sun_altitude_deg)
  local twilight = smoothstep(-14.0, -1.0, frame.sun_altitude_deg) *
    (1.0 - smoothstep(1.0, 12.0, frame.sun_altitude_deg))

  local horizon_r = 0.025 + 0.66 * day + 0.14 * twilight
  local horizon_g = 0.035 + 0.77 * day + 0.07 * twilight
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
      r = r + 0.18 * glow
      g = g + 0.08 * glow
      b = b + 0.015 * glow
    end
  end

  return clamp(r, 0.0, 1.0), clamp(g, 0.0, 1.0), clamp(b, 0.0, 1.0)
end

local sky_dome_directions

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

local sky_dome_packed = {}

function sky_render.draw_dome(event, frame)
  local directions = build_sky_dome_directions()
  local packed = sky_dome_packed
  local cursor = 0
  for index = 1, #directions do
    local direction = directions[index]
    local dx, dy, dz = direction.x, direction.y, direction.z
    local r, g, b = sky_vertex_color(frame, dx, dy, dz)
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

local function body_basis(direction_x, direction_y, direction_z)
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

local function add_pixel_quad(packed, cursor, rx, ry, rz, ux, uy, uz,
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
    return cursor
  end

  cursor = emit(left, bottom)
  cursor = emit(right, bottom)
  cursor = emit(right, top)
  cursor = emit(left, top)
  return cursor
end

local function add_billboard_quad(packed, cursor, dx, dy, dz, radius, half_size,
    red, green, blue, alpha)
  local _, _, _, rx, ry, rz, ux, uy, uz = body_basis(dx, dy, dz)
  local px, py, pz = dx * radius, dy * radius, dz * radius
  local function emit(x_scale, y_scale)
    packed[cursor + 1] = px + rx * x_scale + ux * y_scale
    packed[cursor + 2] = py + ry * x_scale + uy * y_scale
    packed[cursor + 3] = pz + rz * x_scale + uz * y_scale
    packed[cursor + 4] = 0.0
    packed[cursor + 5] = 0.0
    packed[cursor + 6] = red
    packed[cursor + 7] = green
    packed[cursor + 8] = blue
    packed[cursor + 9] = alpha
    cursor = cursor + 9
    return cursor
  end
  cursor = emit(-half_size, -half_size)
  cursor = emit(half_size, -half_size)
  cursor = emit(half_size, half_size)
  cursor = emit(-half_size, half_size)
  return cursor
end

local function draw_pixel_vertices(event, dx, dy, dz, packed, count)
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

local function sun_pixel_color(x, y, animation_phase)
  local edge = math.min(x, CELESTIAL_GRID - 1 - x,
    y, CELESTIAL_GRID - 1 - y)

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
  green = green + light_weight * 0.010 * interior -
    warm_weight * 0.012 * interior
  blue = blue + light_weight * 0.018 * interior -
    warm_weight * 0.020 * interior

  return clamp(red, 0.0, 1.0),
         clamp(green, 0.0, 1.0),
         clamp(blue, 0.0, 1.0)
end

local celestial_packed = {}

function sky_render.draw_sun(event, frame, alpha)
  if alpha <= 0.001 then return end

  local millis = tonumber(frame.utc_millis) or 0.0
  local texture_phase = (millis % SUN_TEXTURE_CYCLE_MS) /
    SUN_TEXTURE_CYCLE_MS * TWO_PI
  local pulse_phase = (millis % SUN_PULSE_CYCLE_MS) /
    SUN_PULSE_CYCLE_MS * TWO_PI

  local animated_half_size = SUN_HALF_SIZE *
    (1.0 + math.sin(pulse_phase) * SUN_PULSE_AMOUNT)
  local brightness = 0.992 + 0.008 * math.sin(pulse_phase + 0.7)

  local dx, dy, dz, rx, ry, rz, ux, uy, uz = body_basis(
    frame.sun_direction_x, frame.sun_direction_y, frame.sun_direction_z)
  local packed = celestial_packed
  local cursor = 0

  for y = 0, CELESTIAL_GRID - 1 do
    for x = 0, CELESTIAL_GRID - 1 do
      local red, green, blue = sun_pixel_color(x, y, texture_phase)
      cursor = add_pixel_quad(packed, cursor, rx, ry, rz, ux, uy, uz, animated_half_size,
        x, y,
        clamp(red * brightness, 0.0, 1.0),
        clamp(green * brightness, 0.0, 1.0),
        clamp(blue * brightness, 0.0, 1.0),
        alpha)
    end
  end

  draw_pixel_vertices(event, dx, dy, dz, packed, cursor)
end

local MOON_CRATERS = {
  { 4.7, 4.8, 1.15, -1 },
  { 10.2, 4.2, 0.85, -2 },
  { 8.7, 8.4, 1.35, -1 },
  { 4.3, 10.3, 0.80, -2 },
  { 11.0, 11.0, 1.05, -1 },
  { 6.4, 12.1, 0.55, -2 },
}

local function moon_pixel_color(x, y, nx, ny, light_dot, near_limb)
  local surface = 0.60 + 0.18 * (-nx) + 0.13 * ny +
    0.22 * math.max(light_dot, 0.0)
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

local function moon_geometry(frame, half_size, alpha, packed)
  local mdx, mdy, mdz = normalize3(
    frame.moon_direction_x, frame.moon_direction_y, frame.moon_direction_z)
  local sdx, sdy, sdz = normalize3(
    frame.sun_direction_x, frame.sun_direction_y, frame.sun_direction_z)

  local separation_cos = clamp(dot3(mdx, mdy, mdz, sdx, sdy, sdz), -1.0, 1.0)
  local separation = math.acos(separation_cos)

  local tx = sdx - mdx * separation_cos
  local ty = sdy - mdy * separation_cos
  local tz = sdz - mdz * separation_cos
  local tangent_length = math.sqrt(tx * tx + ty * ty + tz * tz)

  local rx, ry, rz, ux, uy, uz
  if tangent_length > 1.0e-6 then
    rx, ry, rz = tx / tangent_length, ty / tangent_length, tz / tangent_length
    ux, uy, uz = cross(rx, ry, rz, mdx, mdy, mdz)
    ux, uy, uz = normalize3(ux, uy, uz)
  else
    local _, _, _, brx, bry, brz, bux, buy, buz = body_basis(mdx, mdy, mdz)
    rx, ry, rz, ux, uy, uz = brx, bry, brz, bux, buy, buz
  end

  local sun_side = math.sin(separation)
  local sun_depth = -math.cos(separation)
  local cursor = 0
  local center = (CELESTIAL_GRID - 1) * 0.5
  local radius = 6.55

  for y = 0, CELESTIAL_GRID - 1 do
    for x = 0, CELESTIAL_GRID - 1 do
      local nx = ((x + 0.5) - center) / radius
      local ny = (center - (y + 0.5)) / radius
      local rr = nx * nx + ny * ny
      if rr <= 1.0 then
        local nz = math.sqrt(math.max(0.0, 1.0 - rr))
        local light_dot = nx * sun_side + nz * sun_depth

        if light_dot > 0.035 then
          local red, green, blue = moon_pixel_color(
            x, y, nx, ny, light_dot, rr > 0.80)
          cursor = add_pixel_quad(packed, cursor, rx, ry, rz, ux, uy, uz, half_size,
            x, y, red, green, blue, alpha)
        end
      end
    end
  end
  return mdx, mdy, mdz, cursor, math.deg(separation)
end

local function moon_contrast_visibility(frame, separation_deg)
  local illumination = clamp(frame.moon_illumination or 0.0, 0.0, 1.0)

  if illumination < 0.0015 then return 0.0 end

  local daylight = smoothstep(-6.0, 10.0, frame.sun_altitude_deg or -90.0)
  local night_visibility = smoothstep(0.0008, 0.008, illumination)
  local day_phase_visibility = smoothstep(0.008, 0.080, illumination)
  local day_separation_visibility = smoothstep(8.0, 30.0, separation_deg)
  local day_visibility = day_phase_visibility * day_separation_visibility

  return night_visibility * (1.0 - daylight) + day_visibility * daylight
end

function sky_render.draw_moon(event, frame, alpha, half_size)
  if alpha <= 0.001 then return end
  local packed = celestial_packed
  local mdx, mdy, mdz, cursor, separation_deg =
    moon_geometry(frame, half_size, alpha, packed)
  local visibility = moon_contrast_visibility(frame, separation_deg)
  if visibility <= 0.001 then return end

  if visibility < 0.999 then
    for offset = 9, cursor, 9 do
      packed[offset] = packed[offset] * visibility
    end
  end
  draw_pixel_vertices(event, mdx, mdy, mdz, packed, cursor)
end

local star_directions

local function build_star_directions()
  if star_directions ~= nil then return star_directions end
  local directions = {}
  for index = 1, STAR_COUNT do
    local u = hash01(index * 7 + 1)
    local v = hash01(index * 13 + 3)
    local lon = TWO_PI * u
    local lat = math.asin(clamp(2.0 * v - 1.0, -1.0, 1.0))
    local cos_lat = math.cos(lat)
    directions[index] = {
      x = cos_lat * math.sin(lon),
      y = math.sin(lat),
      z = -cos_lat * math.cos(lon),
      tint = hash01(index * 31 + 7),
      size = 0.70 + hash01(index * 47 + 11) * 0.60,
    }
  end
  star_directions = directions
  return directions
end

local stars_packed = {}

function sky_render.draw_stars(event, frame)
  local brightness = sky_render.star_brightness(frame)
  if brightness <= 0.01 then return end

  local directions = build_star_directions()
  local packed = stars_packed
  local cursor = 0
  local cx = event.camera_x or 0.0
  local cy = event.camera_y or 0.0
  local cz = event.camera_z or 0.0

  for index = 1, #directions do
    local star = directions[index]
    local dx, dy, dz = star.x, star.y, star.z
    if dy > -0.05 then
      local alpha = brightness * (0.45 + 0.55 * star.tint)
      local half_size = STAR_HALF_SIZE * star.size
      local warm = star.tint
      local red = 0.85 + warm * 0.15
      local green = 0.88 + warm * 0.10
      local blue = 1.00 - warm * 0.08
      cursor = add_billboard_quad(packed, cursor, dx, dy, dz, STAR_SPHERE_RADIUS, half_size,
        red, green, blue, alpha)
    end
  end

  if cursor < 36 then return end
  for index = #packed, cursor + 1, -1 do
    packed[index] = nil
  end

  minecraft.render.quads({
    x = cx,
    y = cy,
    z = cz,
    world_space = true,
    blend = true,
    cull = false,
    depth_test = false,
    depth_write = false,
    packed = packed,
  })
end

return sky_render
