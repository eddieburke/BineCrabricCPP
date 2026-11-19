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
  local latitude_steps = 48
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
local sky_dome_packed_built = false
local sky_dome_packed_altitude = 0.0
-- The dome's positions are fixed and its colour tracks only frame.sun_altitude_deg,
-- so 6144 vertices are packed once and recoloured only when the sun has moved.
local SKY_DOME_ALTITUDE_STEP = 0.05

function sky_render.draw_dome(event, frame)
  local directions = build_sky_dome_directions()
  local packed = sky_dome_packed
  local altitude = frame.sun_altitude_deg

  if not sky_dome_packed_built then
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
    sky_dome_packed_built = true
    sky_dome_packed_altitude = altitude
  elseif math.abs(altitude - sky_dome_packed_altitude) >= SKY_DOME_ALTITUDE_STEP then
    local cursor = 0
    for index = 1, #directions do
      local direction = directions[index]
      local r, g, b = sky_vertex_color(frame, direction.x, direction.y, direction.z)
      packed[cursor + 6] = r
      packed[cursor + 7] = g
      packed[cursor + 8] = b
      cursor = cursor + 9
    end
    sky_dome_packed_altitude = altitude
  end
  minecraft.render.quads({
    x = event.eye_x or 0.0,
    y = event.eye_y or 0.0,
    z = event.eye_z or 0.0,
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

local function emit_pixel_vertex(packed, cursor, rx, ry, rz, ux, uy, uz,
    x_scale, y_scale, red, green, blue, alpha)
  packed[cursor + 1] = rx * x_scale + ux * y_scale
  packed[cursor + 2] = ry * x_scale + uy * y_scale
  packed[cursor + 3] = rz * x_scale + uz * y_scale
  packed[cursor + 4] = 0.0
  packed[cursor + 5] = 0.0
  packed[cursor + 6] = red
  packed[cursor + 7] = green
  packed[cursor + 8] = blue
  packed[cursor + 9] = alpha
  return cursor + 9
end

local function add_pixel_quad(packed, cursor, rx, ry, rz, ux, uy, uz,
    half_size, grid_x, grid_y, red, green, blue, alpha)
  local pixel = (half_size * 2.0) / CELESTIAL_GRID
  local overlap = pixel * 0.015
  local left = -half_size + grid_x * pixel - overlap
  local right = -half_size + (grid_x + 1) * pixel + overlap
  local top = half_size - grid_y * pixel + overlap
  local bottom = half_size - (grid_y + 1) * pixel - overlap

  cursor = emit_pixel_vertex(packed, cursor, rx, ry, rz, ux, uy, uz, left, bottom, red, green, blue, alpha)
  cursor = emit_pixel_vertex(packed, cursor, rx, ry, rz, ux, uy, uz, right, bottom, red, green, blue, alpha)
  cursor = emit_pixel_vertex(packed, cursor, rx, ry, rz, ux, uy, uz, right, top, red, green, blue, alpha)
  cursor = emit_pixel_vertex(packed, cursor, rx, ry, rz, ux, uy, uz, left, top, red, green, blue, alpha)
  return cursor
end

local function sky_blend_mode(frame)
  if (frame.sun_altitude_deg or -90.0) > -6.0 then
    return "additive"
  end
  return true
end

local function draw_pixel_vertices(event, dx, dy, dz, packed, cursor, blend_mode)
  if cursor < 36 then return 0 end
  for index = #packed, cursor + 1, -1 do
    packed[index] = nil
  end
  return minecraft.render.quads({
    x = (event.eye_x or 0.0) + dx * SKY_BODY_RADIUS,
    y = (event.eye_y or 0.0) + dy * SKY_BODY_RADIUS,
    z = (event.eye_z or 0.0) + dz * SKY_BODY_RADIUS,
    world_space = true,
    blend = blend_mode or true,
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

  local daylight = smoothstep(-6.0, 10.0, frame.sun_altitude_deg or -90.0)
  local alpha_scale = visibility * (1.0 - 0.30 * daylight)
  if alpha_scale < 0.999 then
    for offset = 9, cursor, 9 do
      packed[offset] = packed[offset] * alpha_scale
    end
  end
  draw_pixel_vertices(event, mdx, mdy, mdz, packed, cursor, sky_blend_mode(frame))
end

local PLANET_GRID = 11
local PLANET_HALF_GRID = (PLANET_GRID - 1) * 0.5
local PLANET_RING_TILT_COS = math.cos(0.38)
local PLANET_RING_TILT_SIN = math.sin(0.38)
local URANUS_RING_TILT_COS = math.cos(1.18)
local URANUS_RING_TILT_SIN = math.sin(1.18)
local planet_packed = {}

local function emit_planet_vertex(packed, cursor, cx, cy, cz, rx, ry, rz, ux, uy, uz,
    x_scale, y_scale, red, green, blue, alpha)
  packed[cursor + 1] = cx + rx * x_scale + ux * y_scale
  packed[cursor + 2] = cy + ry * x_scale + uy * y_scale
  packed[cursor + 3] = cz + rz * x_scale + uz * y_scale
  packed[cursor + 4] = 0.0
  packed[cursor + 5] = 0.0
  packed[cursor + 6] = red
  packed[cursor + 7] = green
  packed[cursor + 8] = blue
  packed[cursor + 9] = alpha
  return cursor + 9
end

local function add_planet_quad(packed, cursor, cx, cy, cz, rx, ry, rz, ux, uy, uz,
    half_size, grid_x, grid_y, red, green, blue, alpha)
  local pixel = (half_size * 2.0) / PLANET_GRID
  local overlap = pixel * 0.018
  local left = -half_size + grid_x * pixel - overlap
  local right = -half_size + (grid_x + 1) * pixel + overlap
  local top = half_size - grid_y * pixel + overlap
  local bottom = half_size - (grid_y + 1) * pixel - overlap

  cursor = emit_planet_vertex(packed, cursor, cx, cy, cz, rx, ry, rz, ux, uy, uz, left, bottom, red, green, blue, alpha)
  cursor = emit_planet_vertex(packed, cursor, cx, cy, cz, rx, ry, rz, ux, uy, uz, right, bottom, red, green, blue, alpha)
  cursor = emit_planet_vertex(packed, cursor, cx, cy, cz, rx, ry, rz, ux, uy, uz, right, top, red, green, blue, alpha)
  cursor = emit_planet_vertex(packed, cursor, cx, cy, cz, rx, ry, rz, ux, uy, uz, left, top, red, green, blue, alpha)
  return cursor
end

local function planet_disc_shade(u, v, z, lx, ly, lz, daylight)
  local diffuse = math.max(0.0, u * lx + v * ly + z * lz)
  local limb = 0.72 + 0.28 * z
  local ambient = 0.28 * (1.0 - 0.86 * daylight)
  return (ambient + (1.0 - ambient) * diffuse) * limb
end

local function ring_pixel(nx, ny, c, s, inner_r, outer_r, gap_r, red, green, blue, base_alpha)
  local px = nx * c + ny * s
  local py = -nx * s + ny * c
  local ry = py / 0.30
  local dist = math.sqrt(px * px + ry * ry)
  if dist < inner_r or dist > outer_r then
    return nil
  end
  if gap_r > 0.0 and math.abs(dist - gap_r) < 0.045 then
    return nil
  end
  local band = 0.88 + 0.12 * math.sin(dist * 24.0)
  return red * band, green * band, blue * band, base_alpha, py <= 0.0
end

local function sample_planet_pixel(id, gx, gy, lx, ly, lz, daylight)
  local nx = (gx - PLANET_HALF_GRID) / PLANET_HALF_GRID
  local ny = (PLANET_HALF_GRID - gy) / PLANET_HALF_GRID

  if id == "saturn" or id == "uranus" then
    local disc_scale = 0.56
    local u = nx / disc_scale
    local v = ny / disc_scale
    local r2 = u * u + v * v
    local rr, rg, rb, ra, in_front
    if id == "saturn" then
      rr, rg, rb, ra, in_front = ring_pixel(nx, ny, PLANET_RING_TILT_COS, PLANET_RING_TILT_SIN,
        0.68, 1.04, 0.88, 0.92, 0.84, 0.66, 0.88)
    else
      rr, rg, rb, ra, in_front = ring_pixel(nx, ny, URANUS_RING_TILT_COS, URANUS_RING_TILT_SIN,
        0.74, 0.96, 0.0, 0.72, 0.92, 0.96, 0.45)
    end
    if rr ~= nil and (in_front or r2 > 1.0) then
      return rr, rg, rb, ra
    end
    if r2 <= 1.0 then
      local z = math.sqrt(math.max(0.0, 1.0 - r2))
      local shade = planet_disc_shade(u, v, z, lx, ly, lz, daylight)
      if id == "saturn" then
        local band = 0.94 + 0.06 * math.cos(v * 9.0)
        return clamp(0.92 * band * shade, 0.0, 1.0),
          clamp(0.81 * band * shade, 0.0, 1.0),
          clamp(0.58 * band * shade, 0.0, 1.0),
          1.0
      else
        local band = 0.95 + 0.05 * math.sin(v * 5.0)
        return clamp(0.56 * band * shade, 0.0, 1.0),
          clamp(0.88 * band * shade, 0.0, 1.0),
          clamp(0.92 * band * shade, 0.0, 1.0),
          1.0
      end
    end
    return nil
  end

  local disc_scale = 0.86
  local u = nx / disc_scale
  local v = ny / disc_scale
  local r2 = u * u + v * v
  if r2 > 1.0 then
    return nil
  end

  local z = math.sqrt(math.max(0.0, 1.0 - r2))
  local shade = planet_disc_shade(u, v, z, lx, ly, lz, daylight)
  local r, g, b

  if id == "mercury" then
    local crater = ((gx * 7 + gy * 13) % 5 == 0) and 0.84 or 1.0
    r, g, b = 0.70 * crater, 0.67 * crater, 0.63 * crater
  elseif id == "venus" then
    local swirl = 0.96 + 0.04 * math.sin(u * 3.2 + v * 2.1)
    r, g, b = 0.98 * swirl, 0.90 * swirl, 0.69 * swirl
  elseif id == "mars" then
    if v > 0.70 and math.abs(u) < 0.45 then
      r, g, b = 0.95, 0.93, 0.90
    else
      local basalt = math.sin(u * 4.5 + 0.6) * math.cos(v * 3.5 - 0.3) > 0.25 and 0.74 or 1.0
      r, g, b = 0.89 * basalt, 0.42 * basalt, 0.24 * basalt
    end
  elseif id == "jupiter" then
    local du = (u - 0.28) / 0.30
    local dv = (v + 0.22) / 0.18
    if du * du + dv * dv <= 1.0 then
      r, g, b = 0.86, 0.36, 0.24
    else
      local stripe = math.cos(v * 11.5)
      if stripe > 0.35 then
        r, g, b = 0.94, 0.88, 0.78
      elseif stripe > -0.25 then
        r, g, b = 0.85, 0.68, 0.50
      else
        r, g, b = 0.74, 0.50, 0.36
      end
    end
  elseif id == "neptune" then
    if math.abs(v - 0.18) < 0.12 and u > -0.45 and u < 0.35 then
      r, g, b = 0.52, 0.76, 1.00
    else
      local band = 0.94 + 0.06 * math.cos(v * 7.0)
      r, g, b = 0.27 * band, 0.50 * band, 0.96 * band
    end
  else
    local hu = (u - 0.14) / 0.36
    local hv = (v + 0.08) / 0.32
    if hu * hu + hv * hv <= 1.0 then
      r, g, b = 0.92, 0.88, 0.82
    elseif v < -0.05 and v > -0.38 and u < 0.0 then
      r, g, b = 0.48, 0.34, 0.28
    else
      r, g, b = 0.80, 0.70, 0.61
    end
  end

  return clamp(r * shade, 0.0, 1.0),
    clamp(g * shade, 0.0, 1.0),
    clamp(b * shade, 0.0, 1.0),
    1.0
end

function sky_render.draw_planets(event, frame, list, count, scale)
  if count <= 0 then return end
  local packed = planet_packed
  local cursor = 0
  local size_scale = scale or 2.0
  local sdx = frame.sun_direction_x
  local sdy = frame.sun_direction_y
  local sdz = frame.sun_direction_z
  local daylight = smoothstep(-6.0, 10.0, frame.sun_altitude_deg or -90.0)
  local day_alpha_scale = 1.0 - 0.18 * daylight

  for i = 1, count do
    local entry = list[i]
    local dx, dy, dz, rx, ry, rz, ux, uy, uz = body_basis(entry.dx, entry.dy, entry.dz)
    local cx = dx * SKY_BODY_RADIUS
    local cy = dy * SKY_BODY_RADIUS
    local cz = dz * SKY_BODY_RADIUS
    local half_size = entry.half_size * size_scale
    local alpha = entry.alpha * day_alpha_scale

    local sr = dot3(sdx, sdy, sdz, rx, ry, rz)
    local su = dot3(sdx, sdy, sdz, ux, uy, uz)
    local proj_len = math.sqrt(sr * sr + su * su)
    local lr = proj_len > 1.0e-4 and (sr / proj_len) or 1.0
    local lu = proj_len > 1.0e-4 and (su / proj_len) or 0.0
    local cos_phase = clamp(2.0 * (entry.illumination or 1.0) - 1.0, -1.0, 1.0)
    local sin_phase = math.sqrt(math.max(0.0, 1.0 - cos_phase * cos_phase))
    local lx = lr * sin_phase
    local ly = lu * sin_phase
    local lz = cos_phase

    for gy = 0, PLANET_GRID - 1 do
      for gx = 0, PLANET_GRID - 1 do
        local r, g, b, pa = sample_planet_pixel(entry.id, gx, gy, lx, ly, lz, daylight)
        if r ~= nil then
          cursor = add_planet_quad(packed, cursor, cx, cy, cz,
            rx, ry, rz, ux, uy, uz, half_size, gx, gy,
            r, g, b, pa * alpha)
        end
      end
    end
  end

  if cursor < 36 then return end
  for index = #packed, cursor + 1, -1 do
    packed[index] = nil
  end
  minecraft.render.quads({
    x = event.eye_x or 0.0,
    y = event.eye_y or 0.0,
    z = event.eye_z or 0.0,
    world_space = true,
    blend = sky_blend_mode(frame),
    cull = false,
    depth_test = false,
    depth_write = false,
    packed = packed,
  })
end

return sky_render
