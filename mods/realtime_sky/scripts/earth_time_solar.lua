local earth_time_solar = {}

local PI = 3.141592653589793
local DEG = PI / 180.0
local clamp = minecraft.util.clamp

local function normalize_tick(tick)
  local value = tick % 24000
  if value < 0 then
    value = value + 24000
  end
  return value
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

local function day_number_ut(utc_millis)
  local seconds = utc_millis / 1000.0
  local days = seconds / 86400.0
  local unix_day = math.floor(days)
  local y = 1970
  local m = 1
  local d = 1 + unix_day
  while d > 365 do
    local leap = (y % 4 == 0 and (y % 100 ~= 0 or y % 400 == 0)) and 1 or 0
    local year_days = 365 + leap
    if d > year_days then
      d = d - year_days
      y = y + 1
    else
      break
    end
  end
  local month_days = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
  if y % 4 == 0 and (y % 100 ~= 0 or y % 400 == 0) then
    month_days[2] = 29
  end
  while d > month_days[m] do
    d = d - month_days[m]
    m = m + 1
  end
  local ut = (utc_millis % 86400000) / 86400000.0
  local d0 = 367 * y - math.floor(7 * (y + math.floor((m + 9) / 12)) / 4) + math.floor(275 * m / 9) + d - 730530
  return d0 + ut
end

local function local_sidereal_time_rad(utc_millis, longitude_deg)
  local d = day_number_ut(utc_millis)
  return ((280.16 + 360.9856235 * d + longitude_deg) % 360.0) * DEG
end

local function horizontal_from_ra_dec_rad(ra, dec, lat_rad, lst_rad)
  local H = lst_rad - ra
  local sin_alt = math.sin(lat_rad) * math.sin(dec) + math.cos(lat_rad) * math.cos(dec) * math.cos(H)
  sin_alt = clamp(sin_alt, -1.0, 1.0)
  local alt_rad = math.asin(sin_alt)
  local az_south = math.atan2(math.sin(H), math.cos(H) * math.sin(lat_rad) - math.tan(dec) * math.cos(lat_rad))
  local az = (math.deg(az_south) + 180.0) % 360.0
  local alt = math.deg(alt_rad)
  if alt_rad > 0.0 then
    local h = alt
    alt = alt + (0.0002967 / math.tan(math.rad(h + 0.00312536 / (h + 0.08901179)))) * (180.0 / PI)
  end
  return az, alt
end

function earth_time_solar.horizontal_from_ra_dec_hours(ra_hours, dec_deg, utc_millis, settings)
  return minecraft.astronomy.horizontal_from_equatorial(
    ra_hours, dec_deg, utc_millis, settings.latitude or 45.0, settings.longitude or 0.0)
end

function earth_time_solar.to_minecraft_ticks(utc_millis, settings)
  local d = day_number_ut(utc_millis)
  local M = (357.5291 + 0.98560028 * d) * DEG
  local L = M + (1.9148 * math.sin(M) + 0.02 * math.sin(2 * M) +
    0.0003 * math.sin(3 * M)) * DEG + (102.9372 * DEG) + PI
  local sun_ra = math.atan(math.sin(L) * math.cos(23.4392911 * DEG), math.cos(L))
  local hour_angle = local_sidereal_time_rad(utc_millis, settings.longitude or 0.0) - sun_ra
  while hour_angle > PI do hour_angle = hour_angle - PI * 2.0 end
  while hour_angle < -PI do hour_angle = hour_angle + PI * 2.0 end
  return normalize_tick(math.floor(6000.0 + hour_angle / PI * 12000.0))
end

function earth_time_solar.skydome_yaw_offset_deg(utc_millis, settings, celestial)
  local sun_az, _ = earth_time_solar.sun_azimuth_altitude(utc_millis, settings)
  local vanilla_az = (celestial or 0.0) * 360.0
  local delta = sun_az - vanilla_az
  while delta <= -180.0 do
    delta = delta + 360.0
  end
  while delta > 180.0 do
    delta = delta - 360.0
  end
  return delta
end

function earth_time_solar.sun_azimuth_altitude(utc_millis, settings)
  local d = day_number_ut(utc_millis)
  local M = (357.5291 + 0.98560028 * d) * DEG
  local L = M + (1.9148 * math.sin(M) + 0.02 * math.sin(2 * M) + 0.0003 * math.sin(3 * M)) * DEG +
            (102.9372 * DEG) + PI
  local dec = math.asin(math.sin(L) * math.sin(23.4392911 * DEG))
  local ra = math.atan2(math.sin(L) * math.cos(23.4392911 * DEG), math.cos(L))
  local lat = clamp(settings.latitude or 45.0, -90.0, 90.0)
  local lon = settings.longitude or 0.0
  return horizontal_from_ra_dec_rad(ra, dec, lat * DEG, local_sidereal_time_rad(utc_millis, lon))
end

function earth_time_solar.build_frame(settings, partial_ticks, utc_millis)
  partial_ticks = partial_ticks or 0.0
  utc_millis = utc_millis or minecraft.time.utc_millis()
  if partial_ticks > 0.0 then
    utc_millis = utc_millis + math.floor(partial_ticks * 50.0)
  end
  local day_tick = earth_time_solar.to_minecraft_ticks(utc_millis, settings)
  local celestial = earth_time_solar.apply_vanilla_celestial_smoothing(day_tick, partial_ticks)
  local skydome_yaw_deg = earth_time_solar.skydome_yaw_offset_deg(utc_millis, settings, celestial)
  local sun_angle = celestial * PI * 2.0
  local moon_angle = sun_angle + PI
  return {
    utc_millis = utc_millis,
    day_tick = day_tick,
    partial_ticks = partial_ticks,
    celestial = celestial,
    skydome_yaw_deg = skydome_yaw_deg,
    color_cycle_phase = earth_time_solar.color_cycle_phase_from_day_tick(day_tick + partial_ticks),
    sun_angle = sun_angle,
    moon_angle = moon_angle,
  }
end

return earth_time_solar
