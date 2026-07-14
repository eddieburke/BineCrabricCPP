local catalog = nil
local compiled_billboards = nil

local BRIGHTEST_MAGNITUDE_IN_CATALOG = -1.5
local ALPHA_OPAQUE_THRESHOLD = 0.0
local ALPHA_TRANSPARENT_THRESHOLD = 6.0

minecraft.settings.register("Northern Stars", {
  { key = "dimmest_magnitude", label = "Dimmest Star Magnitude", kind = "slider", min = 3.0, max = 8.0, step = 0.1, default = 6.5 },
  { key = "min_star_alpha", label = "Minimum Star Brightness", kind = "slider", min = 0.0, max = 0.3, step = 0.01, decimals = 2, default = 0.05 },
  { key = "brightest_star_size", label = "Brightest Star Size", kind = "slider", min = 0.5, max = 3.0, step = 0.1, default = 1.2 },
  { key = "dimmest_star_size", label = "Dimmest Star Size", kind = "slider", min = 0.02, max = 0.5, step = 0.01, decimals = 2, default = 0.1 },
  { key = "magnitude_size_exponent", label = "Size Falloff Curve", kind = "slider", min = 0.1, max = 1.0, step = 0.05, decimals = 2, default = 0.4 },
  { key = "starfield_radius", label = "Starfield Radius", kind = "slider", min = 50, max = 300, step = 5, default = 100.0 },
  { key = "horizon_cull_deg", label = "Horizon Cull Angle", kind = "slider", min = 0.0, max = 5.0, step = 0.05, decimals = 2, default = 0.35 },
  { key = "horizon_fade_end_deg", label = "Horizon Fade End Angle", kind = "slider", min = 0.5, max = 10.0, step = 0.1, decimals = 1, default = 2.0 },
})

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
  local dimmest_magnitude = minecraft.settings.get("dimmest_magnitude") or 6.5
  local magnitude_size_exponent = minecraft.settings.get("magnitude_size_exponent") or 0.4
  local brightest_star_size = minecraft.settings.get("brightest_star_size") or 1.2
  local dimmest_star_size = minecraft.settings.get("dimmest_star_size") or 0.1
  local normalized = (magnitude - BRIGHTEST_MAGNITUDE_IN_CATALOG) /
    (dimmest_magnitude - BRIGHTEST_MAGNITUDE_IN_CATALOG)
  normalized = math.max(0.0, math.min(1.0, normalized))
  local interpolated = normalized ^ magnitude_size_exponent
  local base_size = brightest_star_size -
    (interpolated * (brightest_star_size - dimmest_star_size))
  local variation = (((seed * 1103515245 + 12345) % 65536) / 65535.0 - 0.5) * 0.15
  return base_size * (1.0 + variation)
end

local function smoothstep(edge0, edge1, value)
  if edge0 == edge1 then
    return value < edge0 and 0.0 or 1.0
  end
  local t = (value - edge0) / (edge1 - edge0)
  t = math.max(0.0, math.min(1.0, t))
  return t * t * (3.0 - 2.0 * t)
end

local function magnitude_to_alpha(magnitude)
  local min_star_alpha = minecraft.settings.get("min_star_alpha") or 0.05
  if magnitude <= ALPHA_OPAQUE_THRESHOLD then
    return 1.0
  elseif magnitude >= ALPHA_TRANSPARENT_THRESHOLD then
    return min_star_alpha
  end
  local normalized = (magnitude - ALPHA_OPAQUE_THRESHOLD) /
    (ALPHA_TRANSPARENT_THRESHOLD - ALPHA_OPAQUE_THRESHOLD)
  return math.max(0.0, math.min(1.0, 1.0 - (normalized * (1.0 - min_star_alpha))))
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
  local dimmest_magnitude = minecraft.settings.get("dimmest_magnitude") or 6.5
  local starfield_radius = minecraft.settings.get("starfield_radius") or 100.0
  local horizon_cull_deg = minecraft.settings.get("horizon_cull_deg") or 0.35
  local horizon_fade_end_deg = minecraft.settings.get("horizon_fade_end_deg") or 2.0
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
    return cos_alt * math.sin(az_rad) * starfield_radius,
      math.sin(alt_rad) * starfield_radius,
      -cos_alt * math.cos(az_rad) * starfield_radius
  end
  for index = 1, catalog.count do
    local ra_hours = catalog.right_ascension[index]
    local dec_deg = catalog.declination[index]
    local magnitude = catalog.magnitude[index]
    if magnitude <= dimmest_magnitude then
      seed = seed + 1
      local az, alt = ra_hours * 15.0, dec_deg
      if astronomy then
        az, alt = horizontal_from_equatorial(
          ra_hours, dec_deg, utc_millis, latitude, longitude)
      end
      if not astronomy or alt >= horizon_cull_deg then
        local sx, sy, sz = spherical_to_cartesian(az, alt)
        local horizon_alpha = astronomy and
          smoothstep(horizon_cull_deg, horizon_fade_end_deg, alt) or 1.0
        billboards[#billboards + 1] = {
          x = sx, y = sy, z = sz,
          size = magnitude_to_size(magnitude, seed),
          alpha = magnitude_to_alpha(magnitude) * horizon_alpha,
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
  -- Astronomy-enabled stars were already converted from RA/declination to
  -- local azimuth/altitude in compile_starfield(). Rotating them again by the
  -- sun/skydome angles moves them away from those coordinates and can push
  -- otherwise-visible billboards through the horizon clip.
  local rotation_x = 0.0
  local rotation_y = 0.0
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