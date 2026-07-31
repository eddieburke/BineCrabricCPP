local solar = {}

local PI = math.pi
local TWO_PI = PI * 2.0
local DEG = PI / 180.0
local RAD_TO_DEG = 180.0 / PI
local ARCSEC = DEG / 3600.0
local DAY_MS = 86400000.0
local HOUR_MS = 3600000.0
local MINUTE_MS = 60000.0
local J2000_JD = 2451545.0
local EARTH_RADIUS_KM = 6378.137
local clamp = minecraft.util.clamp

-- ---------------------------------------------------------------------------
-- Angle / tick normalization
-- ---------------------------------------------------------------------------

local function normalize_tick(tick)
  tick = tick % 24000.0
  if tick < 0.0 then tick = tick + 24000.0 end
  return tick
end

local function normalize_deg(angle)
  angle = angle % 360.0
  if angle < 0.0 then angle = angle + 360.0 end
  return angle
end

local function normalize_signed_deg(angle)
  angle = normalize_deg(angle)
  if angle > 180.0 then angle = angle - 360.0 end
  return angle
end

local function normalize_rad(angle)
  angle = (angle + PI) % TWO_PI - PI
  return angle
end

local function smoothstep(edge0, edge1, value)
  if edge0 == edge1 then return value < edge0 and 0.0 or 1.0 end
  local t = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)
end

-- ---------------------------------------------------------------------------
-- Civil calendar
-- ---------------------------------------------------------------------------

local function is_leap_year(year)
  return year % 4 == 0 and (year % 100 ~= 0 or year % 400 == 0)
end

local function days_in_month(year, month)
  if month == 2 then return is_leap_year(year) and 29 or 28 end
  if month == 4 or month == 6 or month == 9 or month == 11 then return 30 end
  return 31
end

local function days_from_civil(year, month, day)
  year = year - (month <= 2 and 1 or 0)
  local era = math.floor(year / 400)
  local year_of_era = year - era * 400
  local shifted_month = month + (month > 2 and -3 or 9)
  local day_of_year = math.floor((153 * shifted_month + 2) / 5) + day - 1
  local day_of_era = year_of_era * 365 + math.floor(year_of_era / 4) -
    math.floor(year_of_era / 100) + day_of_year
  return era * 146097 + day_of_era - 719468
end

local function civil_from_days(days)
  local z = days + 719468
  local era = math.floor(z / 146097)
  local day_of_era = z - era * 146097
  local year_of_era = math.floor((day_of_era - math.floor(day_of_era / 1460) +
    math.floor(day_of_era / 36524) - math.floor(day_of_era / 146096)) / 365)
  local year = year_of_era + era * 400
  local day_of_year = day_of_era - (365 * year_of_era + math.floor(year_of_era / 4) -
    math.floor(year_of_era / 100))
  local shifted_month = math.floor((5 * day_of_year + 2) / 153)
  local day = day_of_year - math.floor((153 * shifted_month + 2) / 5) + 1
  local month = shifted_month + (shifted_month < 10 and 3 or -9)
  year = year + (month <= 2 and 1 or 0)
  return year, month, day
end

local function split_millis(millis)
  local day_index = math.floor(millis / DAY_MS)
  local millis_of_day = millis - day_index * DAY_MS
  local year, month, day = civil_from_days(day_index)
  local hour = math.floor(millis_of_day / HOUR_MS)
  local minute = math.floor((millis_of_day - hour * HOUR_MS) / MINUTE_MS)
  local second_ms = millis_of_day - hour * HOUR_MS - minute * MINUTE_MS
  return year, month, day, hour, minute, second_ms
end

-- ---------------------------------------------------------------------------
-- Time zone / Julian date
-- ---------------------------------------------------------------------------

local function parse_fixed_offset_minutes(time_zone_id)
  if type(time_zone_id) ~= "string" then return 0 end
  local sign, hours, minutes = time_zone_id:match("^GMT([%+%-])(%d%d?):(%d%d)$")
  if sign == nil then
    sign, hours = time_zone_id:match("^GMT([%+%-])(%d%d?)$")
    minutes = "0"
  end
  if sign == nil then return 0 end
  local hour_value = tonumber(hours)
  local minute_value = tonumber(minutes)
  assert(hour_value <= 14 and minute_value <= 59, "realtime_sky: invalid GMT offset")
  local total = hour_value * 60 + minute_value
  assert(total <= 14 * 60, "realtime_sky: invalid GMT offset")
  return sign == "-" and -total or total
end

local function offset_minutes(settings)
  return parse_fixed_offset_minutes(settings.time_zone_id)
end

local function julian_day(utc_millis)
  return utc_millis / DAY_MS + 2440587.5
end

local function julian_centuries(jd)
  return (jd - J2000_JD) / 36525.0
end

-- ---------------------------------------------------------------------------
-- IAU2006 obliquity and IAU2000A nutation
-- ---------------------------------------------------------------------------

local function mean_obliquity_iau2006(t)
  local u = t / 100.0
  return 84381.406 - 4680.93 * u - 1.55 * u * u +
    1999.25 * u * u * u - 51.38 * u * u * u * u -
    249.67 * u * u * u * u * u + 39.05 * u * u * u * u * u * u +
    7.12 * u * u * u * u * u * u * u
end

local function nutation_iau2000(t)
  local t2 = t * t
  local D = normalize_deg(297.8501954 + 445267.1114414 * t -
    0.0018819 * t2 + t2 * t / 545868.0 - t2 * t2 / 113065000.0)
  local M = normalize_deg(357.5291092 + 35999.0502909 * t -
    0.0001536 * t2 + t2 * t / 24490000.0)
  local Mp = normalize_deg(134.9633964 + 477198.8675055 * t +
    0.0087414 * t2 + t2 * t / 69699.0 - t2 * t2 / 14712000.0)
  local F = normalize_deg(93.2720950 + 483202.0175233 * t -
    0.0036539 * t2 - t2 * t / 3526000.0 + t2 * t2 / 863310000.0)
  local Omega = normalize_deg(125.0445550 - 1934.1361849 * t +
    0.0020756 * t2 + t2 * t / 466900.0 - t2 * t2 / 15318000.0)

  local terms = {
    { -172064161, 174666, 0, 0, 0, 0, 1 },
    { -13170906, -13696, 0, 0, 0, 0, 2 },
    { -2276413, -279, 0, 0, 0, 0, 3 },
    { -2074554, 207, 0, 0, 2, -2, 2 },
    { 1475877, -3633, 0, 0, 2, -2, 1 },
    { -516821, 1226, 0, 0, 2, -2, 0 },
    { 711159, -73, 0, 0, 2, 0, 3 },
    { -387298, -367, 0, 0, 2, 0, 2 },
    { -301461, -36, 0, 0, 0, 2, 3 },
    { 215829, -494, 0, 0, 0, 2, 2 },
    { 128227, 137, 0, 0, 0, 2, 1 },
    { 123457, 11, 0, 0, 2, -2, 3 },
    { 156994, 1, 0, 0, 2, 0, 1 },
    { 63110, -63, 0, 0, 0, 2, 0 },
    { -57976, -63, 0, 0, 2, 0, 0 },
    { -59641, -11, 0, 0, 2, 2, 3 },
    { -51613, -42, 0, 0, 2, 2, 2 },
    { 45803, 50, 0, 0, 0, 2, 4 },
    { -6339, 3, 0, 0, 2, -2, 4 },
    { 38571, -3, 0, 0, 0, 0, 4 },
    { -4773, 0, 0, 0, 0, 0, 5 },
    { 3248, 0, 0, 0, 0, 0, 6 },
  }

  local dpsi_asec = 0.0
  local deps_asec = 0.0
  for _, term in ipairs(terms) do
    local arg = term[3] * D + term[4] * M + term[5] * Mp + term[6] * F + term[7] * Omega
    local sin_arg, cos_arg = math.sin(arg * DEG), math.cos(arg * DEG)
    dpsi_asec = dpsi_asec + term[1] * sin_arg
    deps_asec = deps_asec + term[2] * cos_arg
  end

  return dpsi_asec * 0.0001, deps_asec * 0.0001
end

local function true_equator(t)
  local eps0_asec = mean_obliquity_iau2006(t)
  local dpsi_asec, deps_asec = nutation_iau2000(t)
  local eps_asec = eps0_asec + deps_asec
  local eps = eps_asec * ARCSEC
  local dpsi = dpsi_asec * ARCSEC
  return eps, dpsi, eps0_asec * ARCSEC
end

-- ---------------------------------------------------------------------------
-- Solar position (VSOP87 / Meeus)
-- ---------------------------------------------------------------------------

local function solar_position(jd, t)
  local t2 = t * t

  -- L0 uses Julian millennia (tau = T/10); mean anomaly M uses centuries (t).
  local tau = t / 10.0
  local tau2 = tau * tau
  local tau3 = tau2 * tau
  local tau4 = tau2 * tau2
  local tau5 = tau4 * tau

  local L0 = normalize_deg(280.4664567 + 360007.6982779 * tau + 0.03032028 * tau2 +
    tau3 / 49931.0 - tau4 / 152990.0 - tau5 / 1988000.0)
  local M = normalize_deg(357.5291092 + 35999.0502909 * t -
    0.0001536 * t2 + t2 * t / 24490000.0)

  local sinM = math.sin(M * DEG)
  local sin2M = math.sin(2.0 * M * DEG)
  local sin3M = math.sin(3.0 * M * DEG)

  local C = (1.914600 - 0.004817 * t - 0.000014 * t2) * sinM +
    (0.019993 - 0.000101 * t) * sin2M +
    0.000290 * sin3M

  local sun_lon = L0 + C
  local true_anomaly = M + C

  local R = 1.000001018 * (1.0 - 0.01670862 * math.cos(M * DEG) -
    0.000139 * math.cos(2.0 * M * DEG) -
    0.000030 * math.cos(3.0 * M * DEG)) + 0.0000057 * math.cos(true_anomaly * DEG)

  local eps, dpsi = true_equator(t)
  local apparent_sun_lon = sun_lon + dpsi * RAD_TO_DEG - 20.49552 / 3600.0 / R

  return apparent_sun_lon * DEG, true_anomaly * DEG, R
end

local function solar_ra_dec(utc_millis)
  local jd = julian_day(utc_millis)
  local t = julian_centuries(jd)
  local sun_lon_rad = solar_position(jd, t)
  local eps = true_equator(t)

  local sin_lon = math.sin(sun_lon_rad)
  local cos_lon = math.cos(sun_lon_rad)
  local ra = math.atan(sin_lon * math.cos(eps), cos_lon)
  if ra < 0.0 then ra = ra + TWO_PI end
  local dec = math.asin(clamp(math.sin(eps) * sin_lon, -1.0, 1.0))

  return ra, dec
end

-- ---------------------------------------------------------------------------
-- Atmospheric refraction (Bennett 1982)
-- ---------------------------------------------------------------------------

local function apply_refraction(geometric_altitude_deg)
  local h = geometric_altitude_deg
  if h < -5.0 then return h end
  if h > 90.0 then return h end

  local h_rad = h * DEG
  local R_arcmin
  if h >= -0.5 then
    R_arcmin = 1.0 / math.tan(h_rad + 7.31 / (h_rad + 4.4))
  else
    R_arcmin = -1.0 / math.tan(h_rad)
  end

  return h + R_arcmin / 60.0
end

-- ---------------------------------------------------------------------------
-- Lunar position (ELP2000-82 / Meeus Ch. 47)
-- ---------------------------------------------------------------------------

local LON_TERMS = {
  { 0, 0, 1, 0, 6288774 },
  { 2, 0, -1, 0, 1274027 },
  { 2, 0, 0, 0, 658314 },
  { 0, 0, 2, 0, 213618 },
  { 0, 1, 0, 0, -185116 },
  { 0, 0, 0, 2, -114332 },
  { 2, 0, -2, 0, 58793 },
  { 2, -1, -1, 0, 57066 },
  { 2, 0, 1, 0, 53322 },
  { 2, -1, 0, 0, 45758 },
  { 0, 1, -1, 0, -40923 },
  { 1, 0, 0, 0, -34720 },
  { 0, 1, 1, 0, -30383 },
  { 2, 0, 0, -2, 15327 },
  { 0, 0, 1, 2, -12528 },
  { 0, 0, 1, -2, 10980 },
  { 4, 0, -1, 0, 10675 },
  { 0, 0, 3, 0, 10034 },
  { 4, 0, -2, 0, 8548 },
  { 2, 1, -1, 0, -7888 },
  { 2, 1, 0, 0, -6766 },
  { 1, 0, -1, 0, -5163 },
  { 1, 1, 0, 0, 4987 },
  { 2, -1, 1, 0, 4036 },
  { 2, 0, 2, 0, 3994 },
  { 4, 0, 0, 0, 3861 },
  { 2, 0, -3, 0, -3665 },
  { 0, 1, -2, 0, -2689 },
  { 2, 0, -1, 2, -2602 },
  { 2, -1, -2, 0, 2390 },
  { 1, 0, 1, 0, -2348 },
  { 2, -2, 0, 0, 2236 },
  { 0, 1, 2, 0, -2120 },
  { 0, 2, 0, 0, -2069 },
  { 2, -2, -1, 0, 2048 },
  { 2, 0, 1, -2, -1773 },
  { 2, 0, 0, 2, -1595 },
  { 4, -1, -1, 0, 1215 },
  { 0, 0, 2, 2, -1110 },
  { 3, 0, -1, 0, -892 },
  { 2, 1, 1, 0, -810 },
  { 4, -1, -2, 0, 759 },
  { 0, 2, -1, 0, -713 },
  { 2, 2, -1, 0, -700 },
  { 2, 1, -2, 0, 691 },
  { 2, -1, 0, -2, 596 },
  { 4, 0, 1, 0, 549 },
  { 0, 0, 4, 0, 537 },
  { 4, -1, 0, 0, 520 },
  { 1, 0, -2, 0, -487 },
  { 2, 1, 0, -2, -399 },
  { 0, 0, 2, -2, -381 },
  { 1, 1, 1, 0, 351 },
  { 3, 0, -2, 0, -340 },
  { 4, 0, -3, 0, 330 },
  { 2, -1, 2, 0, 327 },
  { 0, 2, 1, 0, -323 },
  { 1, 1, -1, 0, 299 },
  { 2, 0, 3, 0, 294 },
}

local LAT_TERMS = {
  { 0, 0, 0, 1, 5128122 },
  { 0, 0, 1, 1, 280602 },
  { 0, 0, 1, -1, 277693 },
  { 2, 0, 0, -1, 173237 },
  { 2, 0, -1, 1, 55413 },
  { 2, 0, -1, -1, 46271 },
  { 2, 0, 0, 1, 32573 },
  { 0, 0, 2, 1, 17198 },
  { 2, 0, 1, -1, 9266 },
  { 0, 0, 2, -1, 8822 },
  { 2, -1, 0, -1, 8216 },
  { 2, 0, -2, -1, 4324 },
  { 2, 0, 1, 1, 4200 },
  { 2, 1, 0, -1, -3359 },
  { 2, -1, -1, 1, 2463 },
  { 2, -1, 0, 1, 2211 },
  { 2, -1, -1, -1, 2065 },
  { 0, 1, -1, -1, -1870 },
  { 4, 0, -1, -1, 1828 },
  { 0, 1, 0, 1, -1794 },
  { 0, 0, 0, 3, -1749 },
  { 0, 1, -1, 1, -1565 },
  { 1, 0, 0, 1, -1491 },
  { 0, 1, 1, 1, -1475 },
  { 0, 1, 1, -1, -1410 },
  { 0, 1, 0, -1, -1344 },
  { 1, 0, 0, -1, -1335 },
  { 0, 0, 3, 1, 1107 },
  { 4, 0, 0, -1, 1021 },
  { 4, 0, -1, 1, 833 },
  { 0, 0, 1, -3, 777 },
  { 4, 0, -2, 1, 671 },
  { 2, 0, 0, -3, 607 },
  { 2, 0, 1, -3, 596 },
  { 2, -1, 1, 1, 491 },
  { 2, -2, 0, -1, -451 },
  { 0, 0, 2, -3, 439 },
  { 2, 1, -1, 1, 422 },
  { 2, 1, 0, 1, -421 },
  { 4, 0, 0, 1, -366 },
  { 2, -1, 1, -1, -351 },
  { 2, 0, -2, 1, 331 },
}

local DIST_TERMS = {
  { 0, 0, 1, 0, -20905355 },
  { 2, 0, -1, 0, -3699111 },
  { 2, 0, 0, 0, -2955968 },
  { 0, 0, 2, 0, -569925 },
  { 0, 1, 0, 0, 48112 },
  { 0, 0, 0, 2, -30423 },
  { 2, 0, -2, 0, 14889 },
  { 2, -1, -1, 0, -11622 },
  { 2, 0, 1, 0, 9087 },
  { 2, -1, 0, 0, 7180 },
  { 0, 1, -1, 0, -3386 },
  { 1, 0, 0, 0, -2497 },
  { 0, 1, 1, 0, 1979 },
  { 2, 0, 0, -2, 1850 },
  { 0, 0, 1, 2, -1559 },
  { 0, 0, 1, -2, 1319 },
  { 4, 0, -1, 0, 1085 },
  { 0, 0, 3, 0, 960 },
  { 4, 0, -2, 0, 852 },
  { 2, 1, -1, 0, 702 },
  { 2, 1, 0, 0, 630 },
  { 1, 0, -1, 0, 477 },
  { 1, 1, 0, 0, 456 },
  { 2, -1, 1, 0, -417 },
  { 2, 0, 2, 0, -336 },
  { 4, 0, 0, 0, 305 },
  { 2, 0, -3, 0, 291 },
  { 0, 1, -2, 0, 233 },
  { 2, 0, -1, 2, 186 },
  { 2, -1, -2, 0, 160 },
  { 1, 0, 1, 0, -140 },
  { 2, -2, 0, 0, 118 },
  { 0, 1, 2, 0, 99 },
  { 0, 2, 0, 0, 93 },
  { 2, -2, -1, 0, 85 },
  { 2, 0, 1, -2, 82 },
  { 2, 0, 0, 2, 75 },
  { 4, -1, -1, 0, 57 },
  { 0, 0, 2, 2, 47 },
  { 3, 0, -1, 0, 42 },
  { 2, 1, 1, 0, 38 },
  { 4, -1, -2, 0, 36 },
  { 0, 2, -1, 0, 33 },
  { 2, 2, -1, 0, 30 },
  { 2, 1, -2, 0, -28 },
  { 2, -1, 0, -2, -27 },
  { 4, 0, 1, 0, 25 },
  { 0, 0, 4, 0, 23 },
  { 4, -1, 0, 0, 22 },
  { 1, 0, -2, 0, -19 },
  { 2, 1, 0, -2, 16 },
  { 0, 0, 2, -2, 15 },
  { 1, 1, 1, 0, 13 },
  { 3, 0, -2, 0, -11 },
  { 4, 0, -3, 0, 10 },
  { 2, -1, 2, 0, -9 },
  { 0, 2, 1, 0, -8 },
  { 1, 1, -1, 0, 7 },
  { 2, 0, 3, 0, -6 },
  { 2, 0, -1, -2, 5 },
}

local function lunar_periodic_sum(terms, D, M, Mp, F, E, E2)
  local total = 0.0
  for _, term in ipairs(terms) do
    local d = term[1]
    local m = term[2]
    local mp = term[3]
    local ff = term[4]
    local coeff = term[5]
    local e_factor = 1.0
    if math.abs(m) == 1 then e_factor = E
    elseif math.abs(m) == 2 then e_factor = E2 end
    local arg = d * D + m * M + mp * Mp + ff * F
    total = total + e_factor * coeff * math.sin(arg * DEG)
  end
  return total
end

local function lunar_position(utc_millis)
  local jd = julian_day(utc_millis)
  local t = julian_centuries(jd)
  local t2 = t * t
  local t3 = t2 * t

  local Lp = normalize_deg(218.3164591 + 481267.88134236 * t -
    0.0013268 * t2 + t3 / 538841.0 - t3 * t / 65194000.0)
  local D = normalize_deg(297.8501954 + 445267.1114414 * t -
    0.0018819 * t2 + t3 / 545868.0 - t3 * t / 113065000.0)
  local M = normalize_deg(357.5291092 + 35999.0502909 * t -
    0.0001536 * t2 + t3 / 24490000.0)
  local Mp = normalize_deg(134.9633964 + 477198.8675055 * t +
    0.0087414 * t2 + t3 / 69699.0 - t3 * t / 14712000.0)
  local F = normalize_deg(93.2720950 + 483202.0175233 * t -
    0.0036539 * t2 - t3 / 3526000.0 + t3 * t / 863310000.0)

  local A1 = normalize_deg(119.75 + 131.849 * t)
  local A2 = normalize_deg(53.09 + 479264.290 * t)
  local A3 = normalize_deg(313.45 + 481266.484 * t)

  local E = 1.0 - 0.002516 * t - 0.0000074 * t2
  local E2 = E * E

  local sigma_l = lunar_periodic_sum(LON_TERMS, D, M, Mp, F, E, E2)
  local sigma_b = lunar_periodic_sum(LAT_TERMS, D, M, Mp, F, E, E2)
  local sigma_r = lunar_periodic_sum(DIST_TERMS, D, M, Mp, F, E, E2)

  local sinA1 = math.sin(A1 * DEG)
  local sinA2 = math.sin(A2 * DEG)
  local sinA3 = math.sin(A3 * DEG)
  local add_lon = 3958.0 * sinA1 + 1962.0 * math.sin(Lp * DEG - F * DEG) + 318.0 * sinA2
  local add_lat = -2235.0 * math.sin(Lp * DEG) + 382.0 * sinA3 + 175.0 * math.sin(A1 - F * DEG) +
    175.0 * math.sin(A1 + F * DEG) + 127.0 * math.sin(Lp * DEG - Mp * DEG) - 115.0 * math.sin(Lp * DEG + Mp * DEG)

  local moon_lon = Lp + (sigma_l + add_lon) * 0.0001
  local moon_lat = (sigma_b + add_lat) * 0.0001
  local moon_dist = 385000.56 + sigma_r * 0.001

  local eps, dpsi = true_equator(t)
  moon_lon = moon_lon + dpsi * RAD_TO_DEG

  local lon_rad = moon_lon * DEG
  local lat_rad = moon_lat * DEG

  local cos_lat = math.cos(lat_rad)
  local x_ecl = moon_dist * cos_lat * math.cos(lon_rad)
  local y_ecl = moon_dist * cos_lat * math.sin(lon_rad)
  local z_ecl = moon_dist * math.sin(lat_rad)

  local cos_eps = math.cos(eps)
  local sin_eps = math.sin(eps)
  local x_eq = x_ecl
  local y_eq = y_ecl * cos_eps - z_ecl * sin_eps
  local z_eq = y_ecl * sin_eps + z_ecl * cos_eps

  local ra = math.atan(y_eq, x_eq)
  if ra < 0.0 then ra = ra + TWO_PI end
  local dec = math.atan(z_eq, math.sqrt(x_eq * x_eq + y_eq * y_eq))

  return ra, dec, moon_dist, moon_lon, moon_lat
end

-- ---------------------------------------------------------------------------
-- Horizontal coordinates and topocentric moon
-- ---------------------------------------------------------------------------

local function local_sidereal_time_rad(utc_millis, longitude_deg)
  local jd = julian_day(utc_millis)
  local t = julian_centuries(jd)
  local t2 = t * t
  local t3 = t2 * t

  local gmst = normalize_deg(280.460618375 + 360.9856473662860 * (jd - J2000_JD) +
    0.000387933 * t2 - t3 / 38710000.0)

  local dpsi_asec = nutation_iau2000(t)
  local eps0 = mean_obliquity_iau2006(t) * ARCSEC
  local eq_eqox = dpsi_asec * ARCSEC * math.cos(eps0)

  return (gmst + eq_eqox * RAD_TO_DEG + (longitude_deg or 0.0)) * DEG
end

local function horizontal_from_ra_dec(ra, dec, utc_millis, latitude_deg, longitude_deg)
  local latitude = clamp(latitude_deg or 45.0, -90.0, 90.0) * DEG
  local sidereal = local_sidereal_time_rad(utc_millis, longitude_deg or 0.0)
  local hour_angle = normalize_rad(sidereal - ra)
  local sin_altitude = clamp(
    math.sin(latitude) * math.sin(dec) +
    math.cos(latitude) * math.cos(dec) * math.cos(hour_angle), -1.0, 1.0)
  local geometric_altitude = math.deg(math.asin(sin_altitude))
  local apparent_altitude = apply_refraction(geometric_altitude)
  local azimuth = normalize_deg(math.deg(math.atan(
    math.sin(hour_angle),
    math.cos(hour_angle) * math.sin(latitude) - math.tan(dec) * math.cos(latitude))) + 180.0)
  return azimuth, apparent_altitude, hour_angle, geometric_altitude
end

local function moon_topocentric(ra, dec, dist_km, utc_millis, latitude_deg, longitude_deg)
  local sidereal = local_sidereal_time_rad(utc_millis, longitude_deg or 0.0)
  local lat = clamp(latitude_deg or 45.0, -90.0, 90.0) * DEG
  local geodetic_lat = math.atan(0.99664719 * math.tan(lat))

  local cos_lat = math.cos(geodetic_lat)
  local sin_lat = math.sin(geodetic_lat)
  local cos_sid = math.cos(sidereal)
  local sin_sid = math.sin(sidereal)

  local obs_x = cos_lat * cos_sid
  local obs_y = cos_lat * sin_sid
  local obs_z = sin_lat

  local moon_vector_scale = dist_km / EARTH_RADIUS_KM

  local cos_dec = math.cos(dec)
  local sin_dec = math.sin(dec)
  local cos_ra = math.cos(ra)
  local sin_ra = math.sin(ra)

  local m_x = moon_vector_scale * cos_dec * cos_ra
  local m_y = moon_vector_scale * cos_dec * sin_ra
  local m_z = moon_vector_scale * sin_dec

  local t_x = m_x - obs_x
  local t_y = m_y - obs_y
  local t_z = m_z - obs_z

  local topo_ra = math.atan(t_y, t_x)
  if topo_ra < 0.0 then topo_ra = topo_ra + TWO_PI end
  local topo_dec = math.atan(t_z, math.sqrt(t_x * t_x + t_y * t_y))

  return topo_ra, topo_dec
end

local function direction_from_azimuth_altitude(azimuth_deg, altitude_deg)
  local altitude_rad = altitude_deg * DEG
  local azimuth_rad = azimuth_deg * DEG
  local horizontal = math.cos(altitude_rad)
  return horizontal * math.sin(azimuth_rad),
    math.sin(altitude_rad),
    -horizontal * math.cos(azimuth_rad)
end

-- ---------------------------------------------------------------------------
-- Public API
-- ---------------------------------------------------------------------------

function solar.vanilla_celestial_from_day_tick(day_tick)
  local raw = day_tick / 24000.0 - 0.25
  raw = raw - math.floor(raw)
  if raw < 0.0 then raw = raw + 1.0 end
  local curved = 1.0 - (math.cos(raw * PI) + 1.0) / 2.0
  return raw + (curved - raw) / 3.0
end

function solar.color_cycle_phase_from_day_tick(tick)
  return normalize_tick(tick - 6000.0) / 24000.0
end

function solar.parse_utc_offset_ms(time_zone_id)
  return parse_fixed_offset_minutes(time_zone_id) * MINUTE_MS
end

function solar.time_zone_info(settings)
  return {
    offset_minutes = offset_minutes(settings),
  }
end

function solar.resolve_observer_millis(settings, utc_millis)
  utc_millis = utc_millis or minecraft.time.utc_millis()
  if not settings.override_enabled or not (settings.simulate_date or settings.simulate_time) then
    return utc_millis
  end

  local utc_offset = offset_minutes(settings)
  local local_millis = utc_millis + utc_offset * MINUTE_MS
  local year, month, day, hour, minute, second_ms = split_millis(local_millis)

  if settings.simulate_date then
    year = math.floor(clamp(settings.sim_year or year, 1, 9999))
    month = math.floor(clamp(settings.sim_month or month, 1, 12))
    day = math.floor(clamp(settings.sim_day or day, 1, days_in_month(year, month)))
  end
  if settings.simulate_time then
    hour = math.floor(clamp(settings.sim_hour or hour, 0, 23))
    minute = math.floor(clamp(settings.sim_minute or minute, 0, 59))
    second_ms = 0.0
  end

  local target_local = days_from_civil(year, month, day) * DAY_MS +
    hour * HOUR_MS + minute * MINUTE_MS + second_ms
  return target_local - utc_offset * MINUTE_MS
end

function solar.horizontal_from_ra_dec_hours(ra_hours, dec_deg, utc_millis, settings)
  local azimuth, altitude = horizontal_from_ra_dec(
    ra_hours * 15.0 * DEG,
    dec_deg * DEG,
    utc_millis,
    settings.latitude,
    settings.longitude)
  return azimuth, altitude
end

function solar.sun_azimuth_altitude(utc_millis, settings)
  local right_ascension, declination = solar_ra_dec(utc_millis)
  local azimuth, altitude = horizontal_from_ra_dec(
    right_ascension, declination, utc_millis, settings.latitude, settings.longitude)
  return azimuth, altitude
end

function solar.moon_azimuth_altitude(utc_millis, settings)
  local ra, dec, dist = lunar_position(utc_millis)
  local topo_ra, topo_dec = moon_topocentric(ra, dec, dist, utc_millis, settings.latitude, settings.longitude)
  local azimuth, altitude = horizontal_from_ra_dec(
    topo_ra, topo_dec, utc_millis, settings.latitude, settings.longitude)
  return azimuth, altitude
end

function solar.build_frame(settings, partial_ticks, utc_millis)
  local observer_millis = solar.resolve_observer_millis(settings, utc_millis)
  local right_ascension, declination = solar_ra_dec(observer_millis)
  local sun_azimuth, sun_altitude, hour_angle, geometric_altitude = horizontal_from_ra_dec(
    right_ascension,
    declination,
    observer_millis,
    settings.latitude,
    settings.longitude)

  local ha_unwrapped = hour_angle % TWO_PI
  if ha_unwrapped < 0.0 then ha_unwrapped = ha_unwrapped + TWO_PI end
  local day_tick = normalize_tick(6000.0 + ha_unwrapped / TWO_PI * 24000.0)

  local zenith_deg = clamp(90.0 - sun_altitude, 0.0, 180.0)
  local sun_angle = zenith_deg * DEG

  local moon_ra, moon_dec, moon_dist, moon_lon, moon_lat = lunar_position(observer_millis)
  local topo_moon_ra, topo_moon_dec = moon_topocentric(moon_ra, moon_dec, moon_dist,
    observer_millis, settings.latitude, settings.longitude)
  local moon_azimuth, moon_altitude, _, moon_geometric_altitude = horizontal_from_ra_dec(
    topo_moon_ra, topo_moon_dec, observer_millis, settings.latitude, settings.longitude)

  local moon_zenith_deg = clamp(90.0 - moon_altitude, 0.0, 180.0)
  local moon_angle = moon_zenith_deg * DEG

  local jd = julian_day(observer_millis)
  local sun_lon_rad = solar_position(jd, julian_centuries(jd))
  local sun_lon_deg = math.deg(sun_lon_rad)
  local phase_deg = normalize_deg(moon_lon - sun_lon_deg)
  local illuminated_fraction = (1.0 - math.cos(phase_deg * DEG)) * 0.5
  local moon_phase = phase_deg / 360.0

  local sun_direction_x, sun_direction_y, sun_direction_z =
    direction_from_azimuth_altitude(sun_azimuth, sun_altitude)
  local moon_direction_x, moon_direction_y, moon_direction_z =
    direction_from_azimuth_altitude(moon_azimuth, moon_altitude)

  local solar_time_hours = (day_tick / 1000.0 + 6.0) % 24.0
  local zone = solar.time_zone_info(settings)

  return {
    utc_millis = observer_millis,
    day_tick = day_tick,
    partial_ticks = partial_ticks or 0.0,
    celestial = solar.vanilla_celestial_from_day_tick(day_tick),
    skydome_yaw_deg = normalize_signed_deg(180.0 - sun_azimuth),
    color_cycle_phase = solar.color_cycle_phase_from_day_tick(day_tick),
    sun_angle = sun_angle,
    moon_angle = moon_angle,
    sun_azimuth_deg = sun_azimuth,
    sun_altitude_deg = sun_altitude,
    sun_geometric_altitude_deg = geometric_altitude,
    sun_direction_x = sun_direction_x,
    sun_direction_y = sun_direction_y,
    sun_direction_z = sun_direction_z,
    moon_azimuth_deg = moon_azimuth,
    moon_altitude_deg = moon_altitude,
    moon_geometric_altitude_deg = moon_geometric_altitude,
    moon_distance_earth_radii = moon_dist / EARTH_RADIUS_KM,
    moon_illumination = illuminated_fraction,
    moon_phase = moon_phase,
    moon_direction_x = moon_direction_x,
    moon_direction_y = moon_direction_y,
    moon_direction_z = moon_direction_z,
    solar_hour_angle_rad = hour_angle,
    solar_time_hours = solar_time_hours,
    is_daylight = sun_altitude > -0.833,
    twilight = 1.0 - smoothstep(-18.0, 0.0, sun_altitude),
    time_zone_offset_minutes = zone.offset_minutes,
  }
end

return solar
