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
  for index = 1, catalog.count do
    local ra_hours = catalog.right_ascension[index]
    local dec_deg = catalog.declination[index]
    local magnitude = catalog.magnitude[index]
    if magnitude <= DIMMEST_MAGNITUDE_TO_DRAW then
      seed = seed + 1
      local yaw_deg, pitch_deg = ra_hours * 15.0, dec_deg
      if astronomy then
        yaw_deg, pitch_deg = minecraft.astronomy.horizontal_from_equatorial(
          ra_hours, dec_deg, utc_millis, latitude, longitude)
      end
      if not astronomy or pitch_deg >= -0.5 then
        billboards[#billboards + 1] = {
          yaw_deg = yaw_deg,
          pitch_deg = pitch_deg,
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
