-- Real solar position and Minecraft day-time mapping (pure Lua, NOAA-style).
local earth_time_solar = {}

local PI = math.pi
local TWO_PI = PI * 2.0
local DEG = PI / 180.0
local clamp = minecraft.util.clamp

local function normalize_tick(tick)
  tick = tick % 24000
  if tick < 0 then
    tick = tick + 24000
  end
  return tick
end

local function normalize_deg(angle)
  angle = angle % 360.0
  if angle < 0.0 then
    angle = angle + 360.0
  end
  return angle
end

local function normalize_rad(angle)
  while angle < -PI do
    angle = angle + TWO_PI
  end
  while angle >= PI do
    angle = angle - TWO_PI
  end
  return angle
end

-- Julian days since J2000.0 noon (matches unix millis).
local function days_since_j2000(utc_millis)
  return utc_millis / 86400000.0 - 10957.5
end

-- Returns right ascension and declination in radians.
local function solar_ra_dec(utc_millis)
  local d = days_since_j2000(utc_millis)
  local mean_anomaly_deg = normalize_deg(357.52911 + 0.98560028 * d)
  local mean_anomaly = mean_anomaly_deg * DEG
  local mean_longitude_deg = normalize_deg(280.46646 + 0.98564736 * d)
  local ecliptic_longitude_deg = mean_longitude_deg +
    1.914602 * math.sin(mean_anomaly) +
    0.019993 * math.sin(2.0 * mean_anomaly) +
    0.000289 * math.sin(3.0 * mean_anomaly)
  ecliptic_longitude_deg = normalize_deg(ecliptic_longitude_deg)
  local obliquity = (23.439291 - 0.00000036 * d) * DEG
  local ecliptic_lon = ecliptic_longitude_deg * DEG
  local sin_lon = math.sin(ecliptic_lon)
  local cos_lon = math.cos(ecliptic_lon)
  local sin_obl = math.sin(obliquity)
  local cos_obl = math.cos(obliquity)
  local declination = math.asin(clamp(sin_lon * sin_obl, -1.0, 1.0))
  local right_ascension = math.atan(sin_lon * cos_obl, cos_lon)
  if right_ascension < 0.0 then
    right_ascension = right_ascension + TWO_PI
  end
  return right_ascension, declination
end

local function local_sidereal_time_rad(utc_millis, longitude_deg)
  local d = days_since_j2000(utc_millis)
  local lst_deg = normalize_deg(280.46061837 + 360.98564736629 * d + longitude_deg)
  return lst_deg * DEG
end

local function hour_angle_rad(utc_millis, longitude_deg)
  local ra, _ = solar_ra_dec(utc_millis)
  return normalize_rad(local_sidereal_time_rad(utc_millis, longitude_deg) - ra)
end

local function apply_refraction(altitude_deg)
  if altitude_deg <= -1.0 or altitude_deg >= 15.0 then
    return altitude_deg
  end
  local h = math.max(altitude_deg, 0.0)
  return altitude_deg + (0.0002967 / math.tan(math.rad(h + 0.00312536 / (h + 0.08901179)))) * (180.0 / PI)
end

function earth_time_solar.horizontal_from_ra_dec_hours(ra_hours, dec_deg, utc_millis, settings)
  local ra = ra_hours * 15.0 * DEG
  local dec = dec_deg * DEG
  local lat = clamp(settings.latitude or 45.0, -90.0, 90.0) * DEG
  local ha = normalize_rad(local_sidereal_time_rad(utc_millis, settings.longitude or 0.0) - ra)
  local sin_alt = math.sin(lat) * math.sin(dec) + math.cos(lat) * math.cos(dec) * math.cos(ha)
  sin_alt = clamp(sin_alt, -1.0, 1.0)
  local altitude_deg = apply_refraction(math.deg(math.asin(sin_alt)))
  local azimuth_deg = normalize_deg(math.deg(math.atan2(
    math.sin(ha),
    math.cos(ha) * math.sin(lat) - math.tan(dec) * math.cos(lat))) + 180.0)
  return azimuth_deg, altitude_deg
end

function earth_time_solar.apply_vanilla_celestial_smoothing(tick, partial_ticks)
  local i = math.floor(tick % 24000)
  local f1 = (i + (partial_ticks or 0.0)) / 24000.0 - 0.25
  if f1 < 0.0 then
    f1 = f1 + 1.0
  elseif f1 > 1.0 then
    f1 = f1 - 1.0
  end
  local f2 = f1
  f1 = 1.0 - ((math.cos(f1 * PI) + 1.0) / 2.0)
  return f2 + (f1 - f2) / 3.0
end

function earth_time_solar.color_cycle_phase_from_day_tick(tick)
  local phase = (tick - 6000.0) / 24000.0
  while phase < 0.0 do
    phase = phase + 1.0
  end
  while phase >= 1.0 do
    phase = phase - 1.0
  end
  return phase
end

function earth_time_solar.to_minecraft_ticks(utc_millis, settings)
  local ha = hour_angle_rad(utc_millis, settings.longitude or 0.0)
  return normalize_tick(math.floor(6000.0 + ha / PI * 12000.0 + 0.5))
end

function earth_time_solar.sun_azimuth_altitude(utc_millis, settings)
  local lat = clamp(settings.latitude or 45.0, -90.0, 90.0)
  local lon = settings.longitude or 0.0
  local _, dec = solar_ra_dec(utc_millis)
  local lat_rad = lat * DEG
  local ha = hour_angle_rad(utc_millis, lon)
  local sin_alt = math.sin(lat_rad) * math.sin(dec) + math.cos(lat_rad) * math.cos(dec) * math.cos(ha)
  sin_alt = clamp(sin_alt, -1.0, 1.0)
  local altitude_deg = apply_refraction(math.deg(math.asin(sin_alt)))
  local azimuth_deg = normalize_deg(math.deg(math.atan2(
    math.sin(ha),
    math.cos(ha) * math.sin(lat_rad) - math.tan(dec) * math.cos(lat_rad))) + 180.0)
  return azimuth_deg, altitude_deg
end

function earth_time_solar.skydome_yaw_offset_deg(utc_millis, settings, celestial)
  local sun_az, _ = earth_time_solar.sun_azimuth_altitude(utc_millis, settings)
  local vanilla_heading = (celestial or 0.0) * 360.0
  local delta = sun_az - vanilla_heading
  while delta <= -180.0 do
    delta = delta + 360.0
  end
  while delta > 180.0 do
    delta = delta - 360.0
  end
  return delta
end

function earth_time_solar.build_frame(settings, partial_ticks, utc_millis)
  partial_ticks = partial_ticks or 0.0
  utc_millis = utc_millis or minecraft.time.utc_millis()
  local day_tick = earth_time_solar.to_minecraft_ticks(utc_millis, settings)
  local celestial = earth_time_solar.apply_vanilla_celestial_smoothing(day_tick, partial_ticks)
  local sun_az, sun_alt = earth_time_solar.sun_azimuth_altitude(utc_millis, settings)
  return {
    utc_millis = utc_millis,
    day_tick = day_tick,
    partial_ticks = partial_ticks,
    celestial = celestial,
    skydome_yaw_deg = earth_time_solar.skydome_yaw_offset_deg(utc_millis, settings, celestial),
    color_cycle_phase = earth_time_solar.color_cycle_phase_from_day_tick(day_tick + partial_ticks),
    sun_angle = celestial * TWO_PI,
    moon_angle = celestial * TWO_PI + PI,
    sun_azimuth_deg = sun_az,
    sun_altitude_deg = sun_alt,
    is_daylight = sun_alt > -0.833,
  }
end

return earth_time_solar
