local catalog = nil
local compiled_billboards = nil

local BRIGHTEST_MAGNITUDE_IN_CATALOG = -1.5
local DIMMEST_MAGNITUDE_TO_DRAW = 6.5
local ALPHA_OPAQUE_THRESHOLD = 0.0
local ALPHA_TRANSPARENT_THRESHOLD = 6.0
local MIN_STAR_ALPHA = 0.05
local BRIGHTEST_STAR_RENDER_SIZE = 1.2
local DIMMEST_STAR_RENDER_SIZE = 0.1
local MAGNITUDE_SIZE_EXPONENT = 0.4
local compiled_frame_key = nil

local function load_catalog()
  local root, error_message = minecraft.read_nbt_asset("assets/star_catalog.nbt")
  if root == nil then
    minecraft.log("error", "failed to load star catalog NBT: " .. tostring(error_message))
    return nil
  end
  local right_ascension = root.right_ascension_hours
  local declination = root.declination_degrees
  local magnitude = root.magnitude
  local count = math.floor(root.count or 0)
  if root.version ~= 1 or type(right_ascension) ~= "table" or type(declination) ~= "table" or
      type(magnitude) ~= "table" or count ~= #right_ascension or count ~= #declination or
      count ~= #magnitude then
    minecraft.log("error", "star catalog NBT has an invalid schema")
    return nil
  end
  return {
    count = count,
    right_ascension = right_ascension,
    declination = declination,
    magnitude = magnitude,
  }
end

local function magnitude_to_size(magnitude, seed)
  local normalized = (magnitude - BRIGHTEST_MAGNITUDE_IN_CATALOG) /
    (DIMMEST_MAGNITUDE_TO_DRAW - BRIGHTEST_MAGNITUDE_IN_CATALOG)
  normalized = math.max(0.0, math.min(1.0, normalized))
  local interpolated = normalized ^ MAGNITUDE_SIZE_EXPONENT
  local base_size = BRIGHTEST_STAR_RENDER_SIZE -
    (interpolated * (BRIGHTEST_STAR_RENDER_SIZE - DIMMEST_STAR_RENDER_SIZE))
  local variation = (((seed * 1103515245 + 12345) % 65536) / 65535.0 - 0.5) * 0.15
  return base_size * (1.0 + variation)
end

local function magnitude_to_alpha(magnitude)
  if magnitude <= ALPHA_OPAQUE_THRESHOLD then
    return 1.0
  elseif magnitude >= ALPHA_TRANSPARENT_THRESHOLD then
    return MIN_STAR_ALPHA
  end
  local normalized = (magnitude - ALPHA_OPAQUE_THRESHOLD) /
    (ALPHA_TRANSPARENT_THRESHOLD - ALPHA_OPAQUE_THRESHOLD)
  return math.max(0.0, math.min(1.0, 1.0 - (normalized * (1.0 - MIN_STAR_ALPHA))))
end

local function horizontal_from_equatorial(
    ra_hours, dec_degrees, utc_millis, latitude_degrees, longitude_degrees)
  local days = utc_millis / 86400000.0 - 10957.5
  local epoch_year = 2000.0 + days / 365.2422
  local correction = math.rad(3.82394e-5 * (365.2422 * (epoch_year - 2000.0) - days))
  local dec = math.rad(dec_degrees)
  local cos_dec = math.cos(dec)
  local ra = math.rad(ra_hours * 15.0) + (math.abs(cos_dec) < 1.0e-8 and correction
    or correction / cos_dec)
  local mean_anomaly = (356.0470 + 0.9856002585 * days) % 360.0
  local solar_perihelion = 282.9404 + 4.70935e-5 * days
  local utc_hours = (utc_millis % 86400000.0) / 3600000.0
  local lst = math.rad((mean_anomaly + solar_perihelion + 180.0 +
    utc_hours * 15.0 + longitude_degrees) % 360.0)
  local latitude = math.rad(minecraft.util.clamp(latitude_degrees, -90.0, 90.0))
  local hour_angle = lst - ra
  local sin_altitude = minecraft.util.clamp(
    math.sin(latitude) * math.sin(dec) +
    math.cos(latitude) * math.cos(dec) * math.cos(hour_angle), -1.0, 1.0)
  local altitude = math.asin(sin_altitude)
  local azimuth_south = math.atan(
    math.sin(hour_angle),
    math.cos(hour_angle) * math.sin(latitude) - math.tan(dec) * math.cos(latitude))
  return (math.deg(azimuth_south) + 180.0) % 360.0, math.deg(altitude)
end

local function compile_starfield(event)
  if catalog == nil then
    catalog = load_catalog()
  end
  if catalog == nil then
    compiled_billboards = {}
    return
  end
  local billboards = {}
  local seed = 12345
  local astronomy = event and event.astronomy_enabled
  local utc_millis = astronomy and event.astronomy_utc_millis or 0.0
  local latitude = astronomy and event.observer_latitude_deg or 0.0
  local longitude = astronomy and event.observer_longitude_deg or 0.0
  local function spherical_to_cartesian(az_deg, alt_deg)
    local az_rad = math.rad(az_deg)
    local alt_rad = math.rad(alt_deg)
    local cos_alt = math.cos(alt_rad)
    return cos_alt * math.sin(az_rad), math.sin(alt_rad), -cos_alt * math.cos(az_rad)
  end
  for index = 1, catalog.count do
    local ra_hours = catalog.right_ascension[index]
    local dec_deg = catalog.declination[index]
    local magnitude = catalog.magnitude[index]
    if magnitude <= DIMMEST_MAGNITUDE_TO_DRAW then
      seed = seed + 1
      local az, alt = ra_hours * 15.0, dec_deg
      if astronomy then
        az, alt = horizontal_from_equatorial(
          ra_hours, dec_deg, utc_millis, latitude, longitude)
      end
      if not astronomy or alt >= -0.5 then
        local sx, sy, sz = spherical_to_cartesian(az, alt)
        billboards[#billboards + 1] = {
          x = sx, y = sy, z = sz,
          size = magnitude_to_size(magnitude, seed),
          alpha = magnitude_to_alpha(magnitude),
        }
      end
    end
  end
  compiled_billboards = billboards
end

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.stars,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = 150,
}, function(event)
  local frame_key = "static"
  if event.astronomy_enabled then
    frame_key = table.concat({
      math.floor((event.astronomy_utc_millis or 0.0) / 1000.0),
      string.format("%.4f", event.observer_latitude_deg or 0.0),
      string.format("%.4f", event.observer_longitude_deg or 0.0),
    }, ":")
  end
  if compiled_billboards == nil or compiled_frame_key ~= frame_key then
    compile_starfield(event)
    compiled_frame_key = frame_key
  end
  if compiled_billboards == nil or #compiled_billboards == 0 then
    return
  end
  event.cancel_vanilla = true
  local rotation_x = 0.0
  local rotation_y = 0.0
  if event.astronomy_enabled then
    rotation_x = -(event.celestial_angle or 0.0)
    rotation_y = -math.rad(event.sky_yaw_deg or 0.0)
  end
  minecraft.render.billboards({
    brightness = event.star_brightness or 0.0,
    rotation_x_rad = rotation_x,
    rotation_y_rad = rotation_y,
    blend = "additive",
    depth_test = false,
    depth_write = false,
    billboards = compiled_billboards,
  })
end)

minecraft.log("info", "northern_stars loaded")
