local earth_time_solar = minecraft.require("scripts.earth_time_solar")
local places = minecraft.require("scripts.cities")
local globe_ui = minecraft.require("scripts.globe_ui")
local settings_screen_mod = minecraft.require("scripts.settings_screen")

local SETTINGS_SCREEN_ID = "realtime_sky:settings"

local SCREEN_ID = "realtime_sky:globe"
local SEARCH_FIELD = "place_search"
local CONFIG_FILE = "realtime_sky.txt"
local SKY_PROVIDER_PRIORITY = 50

local SETTINGS_DEFAULTS = {
  enabled = false,
  time_zone_id = "GMT",
  latitude = 45.0,
  longitude = 0.0,
  use_dst = true,
  drive_sun = true,
  show_simulate_panel = false,
  override_enabled = false,
  simulate_date = false,
  simulate_time = false,
  sim_year = 2000,
  sim_month = 1,
  sim_day = 1,
  sim_hour = 0,
  sim_minute = 0,
}

local SETTINGS_KEYS = {
  "enabled", "time_zone_id", "latitude", "longitude", "use_dst", "drive_sun", "show_simulate_panel",
  "override_enabled", "simulate_date", "simulate_time",
  "sim_year", "sim_month", "sim_day", "sim_hour", "sim_minute",
}

local SETTINGS_NAMES = {
  enabled = "enabled",
  time_zone_id = "timeZoneId",
  use_dst = "useDst",
  drive_sun = "driveSun",
  show_simulate_panel = "showSimulatePanel",
  override_enabled = "overrideEnabled",
  simulate_date = "simulateDate",
  simulate_time = "simulateTime",
  sim_year = "simYear",
  sim_month = "simMonth",
  sim_day = "simDay",
  sim_hour = "simHour",
  sim_minute = "simMinute",
}

local SETTINGS_ALIASES = {}
for internal_name, file_name in pairs(SETTINGS_NAMES) do
  SETTINGS_ALIASES[file_name] = internal_name
end

local settings = minecraft.util.copy(SETTINGS_DEFAULTS)
local ui = {
  search = "",
  list_scroll = 0,
  selected_index = 1,
  filtered = places.all(),
  globe_x = 10,
  globe_y = 32,
  globe_size = 220,
  globe_yaw = 0.0,
  globe_pitch = 0.0,
  globe_cam = 2.05,
  dragging = false,
  press_x = 0,
  press_y = 0,
  drag_last_x = 0,
  drag_last_y = 0,
  coast_loaded = false,
}

local clamp = minecraft.util.clamp

local function smoothstep(edge0, edge1, value)
  local t = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)
end

local SKY_DOME_RADIUS = 80.0
local SKY_BODY_RADIUS = 75.0
-- Pixel geometry replaces texture quads entirely. This prevents missing or
-- unresolved celestial textures from degrading into solid white/gray squares.
local CELESTIAL_GRID = 16
-- The old Sun only occupied 12 of the 16 procedural pixels, making its
-- effective diameter much smaller than this number suggested. The new artwork
-- fills the complete grid; 3.65 gives it a readable vanilla-like angular size
-- without returning to the enormous hard-coded vanilla celestial quad.
local SUN_HALF_SIZE = 3.65
-- Slow, low-amplitude animation. The texture motion takes about 18 seconds to
-- complete a cycle and the size changes by only one percent.
local SUN_TEXTURE_CYCLE_MS = 18000.0
local SUN_PULSE_CYCLE_MS = 8000.0
local SUN_PULSE_AMOUNT = 0.010
local MOON_MEAN_HALF_SIZE = 2.00
local TWO_PI = math.pi * 2.0

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

local function sky_color_for_frame(frame)
  local alt = frame.sun_altitude_deg
  local day = smoothstep(-6.0, 12.0, alt)
  local warm = smoothstep(-12.0, -1.0, alt) * (1.0 - smoothstep(1.0, 14.0, alt))
  return 0.015 + (0.45 - 0.015) * day + 0.28 * warm,
    0.025 + (0.65 - 0.025) * day + 0.08 * warm,
    0.070 + (1.00 - 0.070) * day + 0.02 * warm
end

local function sky_vertex_color(frame, x, y, z)
  local zenith_r, zenith_g, zenith_b = sky_color_for_frame(frame)
  local day = smoothstep(-8.0, 10.0, frame.sun_altitude_deg)
  local twilight = smoothstep(-14.0, -1.0, frame.sun_altitude_deg) *
    (1.0 - smoothstep(1.0, 12.0, frame.sun_altitude_deg))

  -- Brighter near the horizon by day, deep blue below it at night.
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

  -- Compact sunrise/sunset glow in the Sun's horizontal direction. This keeps
  -- the custom dome close to vanilla's visual cue without drawing another
  -- oversized translucent disc.
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
      -- Winding is irrelevant because culling is disabled, but keep every quad
      -- ordered consistently for runtimes that still inspect face orientation.
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

local function draw_sky_dome(event, frame)
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

local function dot3(ax, ay, az, bx, by, bz)
  return ax * bx + ay * by + az * bz
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
  end

  cursor = emit(left, bottom)
  cursor = emit(right, bottom)
  cursor = emit(right, top)
  cursor = emit(left, top)
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

  -- Three close tones keep the design simple and intentionally low contrast.
  local red, green, blue
  if edge == 0 then
    red, green, blue = 1.000, 0.925, 0.610
  elseif edge == 1 then
    red, green, blue = 1.000, 0.958, 0.720
  else
    red, green, blue = 1.000, 0.978, 0.800
  end

  -- Two broad patches move continuously instead of switching hard animation
  -- frames. Their maximum contrast is under three percent.
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

  -- Leave the outside border nearly stable so the silhouette never crawls.
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

local function draw_procedural_sun(event, frame, alpha)
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

local function draw_procedural_moon(event, frame, alpha, half_size)
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

local solar_frame_cache
local fit_text

local function clamp_settings()
  settings.latitude = clamp(settings.latitude, -90.0, 90.0)
  settings.longitude = clamp(settings.longitude, -180.0, 180.0)
  settings.sim_year = math.floor(clamp(settings.sim_year, 1, 9999))
  settings.sim_month = math.floor(clamp(settings.sim_month, 1, 12))
  settings.sim_day = math.floor(clamp(settings.sim_day, 1, 31))
  settings.sim_hour = math.floor(clamp(settings.sim_hour, 0, 23))
  settings.sim_minute = math.floor(clamp(settings.sim_minute, 0, 59))
end

local function normalize_time_zone_id(value)
  if type(value) ~= "string" then
    return "GMT"
  end
  value = value:gsub("^%s+", ""):gsub("%s+$", "")
  if value == "" then
    return "GMT"
  end
  value = value:gsub("\\", "/")
  while value:sub(1, 1) == "/" do
    value = value:sub(2)
  end
  return value
end

local function save_settings()
  solar_frame_cache = nil
  settings.time_zone_id = normalize_time_zone_id(settings.time_zone_id)
  minecraft.config.save(CONFIG_FILE, settings, {
    keys = SETTINGS_KEYS,
    names = SETTINGS_NAMES,
    separator = ":",
  })
end

local function load_settings()
  local loaded, found = minecraft.config.load(CONFIG_FILE, SETTINGS_DEFAULTS, {
    aliases = SETTINGS_ALIASES,
  })
  -- Keep the table identity stable. settings_screen.register() holds this exact
  -- table; replacing it made the UI edit an abandoned copy after the first save.
  for key in pairs(settings) do
    settings[key] = nil
  end
  for key, value in pairs(loaded) do
    settings[key] = value
  end
  settings.time_zone_id = normalize_time_zone_id(settings.time_zone_id)
  clamp_settings()
  if not found then
    save_settings()
  end
end

local function realtime_active()
  return settings.enabled == true
end

local function utc_millis()
  return minecraft.time.utc_millis()
end

local function current_solar_frame(partial_ticks)
  local now = utc_millis()
  if solar_frame_cache == nil or now < solar_frame_cache.sample_millis or
      now - solar_frame_cache.sample_millis >= 50.0 then
    solar_frame_cache = earth_time_solar.build_frame(settings, partial_ticks or 0.0, now)
    solar_frame_cache.sample_millis = now
  end
  return solar_frame_cache
end

local function ensure_coastlines()
  if ui.coast_loaded then
    return
  end
  local coast_text = minecraft.read_asset("assets/globe_coasts.txt")
  if coast_text and coast_text ~= "" then
    globe_ui.load_coastlines(coast_text)
    ui.coast_loaded = true
  end
end

local function refresh_filter()
  ui.filtered = places.filter(ui.search)
  if ui.selected_index > #ui.filtered then
    ui.selected_index = math.max(1, #ui.filtered)
  end
  if ui.selected_index < 1 then
    ui.selected_index = 1
  end
end

local function place_label(place, max_width)
  if place.country and #place.country > 0 then
    return fit_text(place.name .. " (" .. place.country .. ")", max_width)
  end
  return fit_text(place.name, max_width)
end

local function frame_globe_to(lat, lon)
  -- The software globe rotates the surface itself. These signs place the
  -- selected location at the center without the old half-angle drift.
  ui.globe_yaw = -lon
  ui.globe_pitch = clamp(-lat, -89.0, 89.0)
  ui.globe_cam = 2.05
end

local function apply_place(place)
  settings.latitude = place.lat
  settings.longitude = place.lon
  settings.time_zone_id = normalize_time_zone_id(place.time_zone_id)
  clamp_settings()
  frame_globe_to(place.lat, place.lon)
  save_settings()
end

local function list_column_left(width)
  return math.floor(width / 2) + 6
end

local function list_column_width(width)
  return width - list_column_left(width) - 10
end

local function list_top()
  return 76
end

local function list_bottom(height)
  return height - 32
end

local function visible_rows(height)
  return math.max(1, math.floor((list_bottom(height) - list_top()) / 14))
end

local function layout(width, height)
  local globe_left = 8
  local globe_top = 34
  local globe_right = list_column_left(width) - 8
  local globe_bottom = height - 32
  local available_width = math.max(1, globe_right - globe_left)
  local available_height = math.max(1, globe_bottom - globe_top)
  ui.globe_size = math.max(1, math.min(available_width, available_height))
  ui.globe_x = globe_left + math.floor((available_width - ui.globe_size) / 2)
  ui.globe_y = globe_top + math.floor((available_height - ui.globe_size) / 2)
  local left = list_column_left(width)
  local col_w = list_column_width(width)
  local control_w = math.floor((col_w - 12) / 4)
  ui.search_x = left
  ui.search_y = 36
  ui.search_w = col_w
  ui.search_h = 18
  ui.dst_toggle_y = height - 24
  ui.dst_toggle_x = left + control_w + 4
  ui.dst_toggle_w = control_w
  ui.dst_toggle_h = 20
  ui.settings_x = left + (control_w + 4) * 2
  ui.settings_w = control_w
  ui.done_x = left + (control_w + 4) * 3
  ui.done_w = col_w - (control_w + 4) * 3
end

local function format_lat_lon(lat, lon)
  local ns = lat >= 0 and "N" or "S"
  local ew = lon >= 0 and "E" or "W"
  return string.format("%.2f %s  %.2f %s", math.abs(lat), ns, math.abs(lon), ew)
end

fit_text = function(text, max_width)
  if minecraft.gui.text_width(text) <= max_width then
    return text
  end
  for length = #text, 1, -1 do
    local candidate = text:sub(1, length) .. "..."
    if minecraft.gui.text_width(candidate) <= max_width then
      return candidate
    end
  end
  return "..."
end

local function draw_list(width, height, mouse_x, mouse_y)
  local left = list_column_left(width)
  local top = list_top()
  local bottom = list_bottom(height)
  local col_w = list_column_width(width)
  local rows = visible_rows(height)

  if ui.list_scroll > math.max(0, #ui.filtered - rows) then
    ui.list_scroll = math.max(0, #ui.filtered - rows)
  end
  if ui.list_scroll < 0 then
    ui.list_scroll = 0
  end

  minecraft.gui.draw_text(left, 22, "LOCATIONS", 0xFF7EC8F2)
  local count_text = tostring(#ui.filtered) .. " places"
  minecraft.gui.draw_text(left, 59, count_text, 0xFF9BB0C2)
  local zone_text = fit_text("Zone  " .. settings.time_zone_id, col_w - minecraft.gui.text_width(count_text) - 10)
  minecraft.gui.draw_text(left + col_w - minecraft.gui.text_width(zone_text), 59, zone_text, 0xFFFFD36A)

  for row = 0, rows - 1 do
    local index = ui.list_scroll + row + 1
    if index > #ui.filtered then
      break
    end
    local place = ui.filtered[index]
    local y = top + row * 14
    local hover = mouse_x >= left and mouse_x < left + col_w and mouse_y >= y and mouse_y < y + 14
    local selected = index == ui.selected_index
    if selected or hover then
      minecraft.gui.fill_rect(left - 1, y - 1, col_w + 2, 16, selected and 0xFF404070 or 0xFF303848)
    end
    minecraft.gui.draw_text(left, y, place_label(place, col_w - 4), selected and 0xFFFFFFFF or 0xFFC0C0C0)
  end

  if #ui.filtered == 0 then
    minecraft.gui.draw_text(left, top + 4, "(no matches)", 0xFFFF8080)
  end
end

local function draw_screen_chrome(width, height)
  local left = list_column_left(width)
  local col_w = list_column_width(width)
  minecraft.gui.fill_rect(4, 4, width - 8, 24, 0xD9121A25)
  minecraft.gui.fill_rect(left - 5, 30, col_w + 10, height - 60, 0xB80D141D)
  minecraft.gui.fill_rect(left - 5, 30, 2, height - 60, 0xFF31536B)
  minecraft.gui.draw_text(10, 12, "REAL-TIME SKY", 0xFFFFFFFF)
  local zoom_label = string.format(
    "GLOBE  drag to rotate  |  wheel to zoom  |  %.1fx",
    globe_ui.zoom_factor(ui))
  minecraft.gui.draw_text(ui.globe_x, 22,
    fit_text(zoom_label, ui.globe_size), 0xFF9BB0C2)
end

local function draw_globe_overlay(width)
  local bar_h = 28
  local y0 = ui.globe_y + ui.globe_size - bar_h
  minecraft.gui.fill_rect(ui.globe_x, y0, ui.globe_size, bar_h, 0xC0202020)
  local coords = format_lat_lon(settings.latitude, settings.longitude)
  minecraft.gui.draw_centered_text(ui.globe_x, y0 + 3, ui.globe_size, coords, 0xFFFFFFFF)
  local frame = current_solar_frame(0.0)
  local sun_line = string.format("Sun %03.0f deg   %+.1f deg", frame.sun_azimuth_deg, frame.sun_altitude_deg)
  minecraft.gui.draw_centered_text(ui.globe_x, y0 + 15, ui.globe_size,
    fit_text(sun_line, ui.globe_size - 8), 0xFF88EE88)
end

local function draw_globe_chrome()
  local border = 2
  minecraft.gui.fill_rect(ui.globe_x - border, ui.globe_y - border, ui.globe_size + border * 2, border, 0xFF505860)
  minecraft.gui.fill_rect(ui.globe_x - border, ui.globe_y + ui.globe_size, ui.globe_size + border * 2, border, 0xFF505860)
  minecraft.gui.fill_rect(ui.globe_x - border, ui.globe_y, border, ui.globe_size, 0xFF505860)
  minecraft.gui.fill_rect(ui.globe_x + ui.globe_size, ui.globe_y, border, ui.globe_size, 0xFF505860)
end

local open_globe_screen

load_settings()
ensure_coastlines()

minecraft.screen.on_ui(minecraft.screen.ids.world_settings,
  minecraft.screen.regions.screen, function(event)
  if event.ui == nil then return event end
  local function realtime_label()
    return settings.enabled and "Realtime: ON" or "Realtime: OFF"
  end
  event.ui:add_centered_button(143,
    realtime_label(),
    function()
      settings.enabled = not settings.enabled
      save_settings()
    end,
    realtime_label)
  return event
end, 100)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.sky,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = SKY_PROVIDER_PRIORITY + 1000,
  when = function()
    return realtime_active()
  end,
}, function(event)
  local frame = current_solar_frame(event.tick_delta)
  event.astronomy_enabled = true
  event.astronomy_utc_millis = frame.utc_millis
  event.observer_latitude_deg = settings.latitude
  event.observer_longitude_deg = settings.longitude

  event.solar_day_tick = frame.day_tick
  event.solar_time_hours = frame.solar_time_hours
  if settings.drive_sun then
    event.celestial_angle = frame.sun_angle
    event.celestial = frame.celestial
    event.sky_yaw_deg = frame.skydome_yaw_deg
    event.sun_direction_x = frame.sun_direction_x
    event.sun_direction_y = frame.sun_direction_y
    event.sun_direction_z = frame.sun_direction_z
    event.sun_azimuth_deg = frame.sun_azimuth_deg
    event.sun_altitude_deg = frame.sun_altitude_deg
    event.moon_direction_x = frame.moon_direction_x
    event.moon_direction_y = frame.moon_direction_y
    event.moon_direction_z = frame.moon_direction_z
    event.moon_azimuth_deg = frame.moon_azimuth_deg
    event.moon_altitude_deg = frame.moon_altitude_deg
    event.moon_illumination = frame.moon_illumination
    event.moon_phase = frame.moon_phase
    event.cancel_vanilla = true
    draw_sky_dome(event, frame)
  end
end)

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.sky,
  is_overworld = true,
  priority = SKY_PROVIDER_PRIORITY,
  when = function()
    return realtime_active()
  end,
}, function(event)
  local frame = current_solar_frame(event.partial_ticks)
  local alt = frame.sun_altitude_deg
  local day = smoothstep(-6.0, 12.0, alt)
  local warm = smoothstep(-12.0, -1.0, alt) * (1.0 - smoothstep(1.0, 14.0, alt))
  event.r = 0.015 + (0.45 - 0.015) * day + 0.28 * warm
  event.g = 0.025 + (0.65 - 0.025) * day + 0.08 * warm
  event.b = 0.070 + (1.00 - 0.070) * day + 0.02 * warm
end)

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.fog,
  is_overworld = true,
  priority = SKY_PROVIDER_PRIORITY,
  when = function()
    return realtime_active()
  end,
}, function(event)
  local frame = current_solar_frame(event.partial_ticks)
  local alt = frame.sun_altitude_deg
  local day = smoothstep(-6.0, 10.0, alt)
  local warm = smoothstep(-12.0, -1.0, alt) * (1.0 - smoothstep(1.0, 12.0, alt))
  event.r = 0.025 + (0.75 - 0.025) * day + 0.20 * warm
  event.g = 0.035 + (0.85 - 0.035) * day + 0.06 * warm
  event.b = 0.080 + (1.00 - 0.080) * day
end)

open_globe_screen = function()
  minecraft.screen.open(SCREEN_ID, { title = "" })
end

minecraft.screen.on_ui(minecraft.screen.ids.mod_settings, minecraft.screen.regions.footer, function(event)
  if event.ui ~= nil then
    event.ui:add_stacked_centered_button("Sky Globe...", open_globe_screen)
  end
  return event
end, 90)

minecraft.on(minecraft.events.screen_event, { screen_id = SCREEN_ID, priority = 100 }, function(event)
  if event.phase == "init" then
    layout(event.width, event.height)
    refresh_filter()
    frame_globe_to(settings.latitude, settings.longitude)
    local button_y = event.height - 24
    minecraft.screen.add_field(SEARCH_FIELD, ui.search_x, ui.search_y, ui.search_w, ui.search_h, {
      text = ui.search,
      max_len = 32,
    })
    minecraft.screen.add_button(ui.done_x, button_y, ui.done_w, 20, "Done", function()
      save_settings()
      minecraft.screen.close()
    end)
    minecraft.screen.add_button(ui.settings_x, button_y, ui.settings_w, 20, "Settings", function()
      save_settings()
      minecraft.screen.open(SETTINGS_SCREEN_ID, { title = "" })
    end)
  elseif event.phase == "render" then
    ensure_coastlines()
    layout(event.width, event.height)
    local query = minecraft.screen.field_text(SEARCH_FIELD) or ""
    if query ~= ui.search then
      ui.search = query
      ui.list_scroll = 0
      ui.selected_index = 1
      refresh_filter()
    end
    if ui.dragging then
      local dx = event.mouse_x - ui.drag_last_x
      local dy = event.mouse_y - ui.drag_last_y
      if dx ~= 0 or dy ~= 0 then
        local sens = globe_ui.drag_degrees_per_pixel(ui)
        ui.globe_yaw = ui.globe_yaw + dx * sens
        ui.globe_pitch = ui.globe_pitch - dy * sens
        ui.globe_pitch = clamp(ui.globe_pitch, -89.0, 89.0)
        ui.drag_last_x = event.mouse_x
        ui.drag_last_y = event.mouse_y
      end
    end
    draw_screen_chrome(event.width, event.height)
    globe_ui.draw(ui, event.width, event.height, settings.latitude, settings.longitude)
    draw_globe_chrome()
    draw_globe_overlay(event.width)
    draw_list(event.width, event.height, event.mouse_x, event.mouse_y)
    minecraft.gui.draw_toggle({ x = ui.dst_toggle_x, y = ui.dst_toggle_y, width = ui.dst_toggle_w,
      height = ui.dst_toggle_h, label = "DST", value = settings.use_dst, mouse_x = event.mouse_x, mouse_y = event.mouse_y })
  elseif event.phase == "mouse" then
    if event.released then
      if ui.dragging and event.button == 0 then
        local dx = event.x - ui.press_x
        local dy = event.y - ui.press_y
        if dx * dx + dy * dy <= 9 then
          local picked = globe_ui.pick_lat_lon(ui, event.width, event.height, event.x, event.y)
          if picked then
            settings.latitude = picked.lat
            settings.longitude = picked.lon
            clamp_settings()
            save_settings()
          end
        end
      end
      ui.dragging = false
      return
    end
    if event.button ~= 0 then
      return
    end
    if minecraft.util.in_rect(event.x, event.y, ui.dst_toggle_x, ui.dst_toggle_y, ui.dst_toggle_w, ui.dst_toggle_h) then
      settings.use_dst = not settings.use_dst
      save_settings()
      event.handled = true
      return
    end
    if globe_ui.contains_point(ui, event.x, event.y) then
      ui.dragging = true
      ui.press_x = event.x
      ui.press_y = event.y
      ui.drag_last_x = event.x
      ui.drag_last_y = event.y
      event.handled = true
      return
    end
    local left = list_column_left(event.width)
    local top = list_top()
    local bottom = list_bottom(event.height)
    local col_w = list_column_width(event.width)
    if event.x >= left and event.x < left + col_w and event.y >= top and event.y < bottom then
      local row = math.floor((event.y - top) / 14)
      local index = ui.list_scroll + row + 1
      if index >= 1 and index <= #ui.filtered then
        ui.selected_index = index
        apply_place(ui.filtered[index])
      end
      event.handled = true
    end
  elseif event.phase == "scroll" then
    if event.x >= list_column_left(event.width) then
      local rows = visible_rows(event.height)
      if event.delta > 0 then
        ui.list_scroll = ui.list_scroll - 3
      else
        ui.list_scroll = ui.list_scroll + 3
      end
      if ui.list_scroll < 0 then
        ui.list_scroll = 0
      end
      local max_scroll = math.max(0, #ui.filtered - rows)
      if ui.list_scroll > max_scroll then
        ui.list_scroll = max_scroll
      end
      event.handled = true
      return
    end
    if globe_ui.contains_point(ui, event.x, event.y) then
      globe_ui.zoom(ui, event.delta)
      event.handled = true
    end
  elseif event.phase == "key" then
    if event.key == minecraft.keys.escape then
      save_settings()
      minecraft.screen.close()
      event.handled = true
    end
  elseif event.phase == "close" then
    save_settings()
    globe_ui.cleanup()
  end
end)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.stars,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = 30,
  when = function()
    return realtime_active()
  end,
}, function(event)
  local frame = current_solar_frame(event.tick_delta)
  local darkness = 1.0 - smoothstep(-18.0, -4.0, frame.sun_altitude_deg)
  event.star_brightness = darkness * darkness * 0.5
end)

-- Draw the physically positioned Sun and Moon at the first guaranteed stage
-- after stars. The custom star renderer cancels the vanilla stars stage; some
-- runtime builds consequently omit stars/after callbacks. terrain_opaque/before
-- still has a valid world draw context, runs after all star rendering, and lets
-- terrain render over the celestial bodies normally.
minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.terrain_opaque,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = -1000,
  when = function()
    return realtime_active()
  end,
}, function(event)
  local frame = current_solar_frame(event.tick_delta)
  local rain_alpha = 1.0 - clamp(event.rain_strength or 0.0, 0.0, 1.0) * 0.80

  local sun_altitude = frame.sun_altitude_deg or -90.0
  local sun_alpha = smoothstep(-1.15, 0.35, sun_altitude) * rain_alpha
  draw_procedural_sun(event, frame, sun_alpha)

  local moon_altitude = frame.moon_altitude_deg or -90.0
  local moon_alpha = smoothstep(-0.35, 1.25, moon_altitude) * rain_alpha
  if moon_alpha > 0.001 then
    local distance_scale = clamp(60.27 /
      math.max(frame.moon_distance_earth_radii or 60.27, 1.0), 0.90, 1.12)
    draw_procedural_moon(event, frame, moon_alpha,
      MOON_MEAN_HALF_SIZE * distance_scale)
  end
end)

local function nearest_world_time_with_phase(world_time, target_tick)
  -- Preserve the current Minecraft day number while choosing the occurrence of
  -- the requested phase nearest to the current absolute world time. This makes
  -- the 23999 -> 0 wrap advance one day instead of jumping backward.
  local day_start = math.floor(world_time / 24000.0) * 24000.0
  local candidate = day_start + target_tick
  local delta = candidate - world_time
  if delta >= 12000.0 then
    candidate = candidate - 24000.0
  elseif delta < -12000.0 then
    candidate = candidate + 24000.0
  end
  return math.floor(candidate + 0.5)
end

-- Keep the actual world phase locked to the same apparent-solar tick used by
-- the visible sun. Minecraft normally advances 24000 ticks in 20 minutes, so a
-- real-time sky must correct that automatic increment. Only the time-of-day
-- phase is replaced; the nearest absolute day is retained across midnight.
minecraft.on(minecraft.events.world_tick, { before = false }, function(event)
  if not realtime_active() or event.remote then
    return
  end

  local frame = current_solar_frame(0.0)
  local target_tick = math.floor(frame.day_tick + 0.5) % 24000
  local world_time = minecraft.world.get_time()
  local synchronized_time = nearest_world_time_with_phase(world_time, target_tick)

  if synchronized_time ~= world_time then
    minecraft.world.set_time(synchronized_time)
  end
end)

settings_screen_mod.register(settings, function()
  save_settings()
end)


minecraft.log("info", "realtime_sky loaded")
