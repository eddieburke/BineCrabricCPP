local earth_time_solar = {}

local PI = math.pi
local TWO_PI = PI * 2.0
local DEG = PI / 180.0
local DAY_MS = 86400000.0
local HOUR_MS = 3600000.0
local MINUTE_MS = 60000.0
local clamp = minecraft.util.clamp

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

local function is_leap_year(year)
  return year % 4 == 0 and (year % 100 ~= 0 or year % 400 == 0)
end

local function days_in_month(year, month)
  if month == 2 then return is_leap_year(year) and 29 or 28 end
  if month == 4 or month == 6 or month == 9 or month == 11 then return 30 end
  return 31
end

-- Sunday = 0, Monday = 1, ... Saturday = 6.
local function day_of_week(year, month, day)
  local offsets = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 }
  if month < 3 then year = year - 1 end
  return (year + math.floor(year / 4) - math.floor(year / 100) +
    math.floor(year / 400) + offsets[month] + day) % 7
end

local function nth_sunday(year, month, occurrence)
  local first = 1 + (7 - day_of_week(year, month, 1)) % 7
  return first + (occurrence - 1) * 7
end

local function last_sunday(year, month)
  local last = days_in_month(year, month)
  return last - day_of_week(year, month, last)
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

-- Named zones used by the bundled city database. This is deliberately explicit:
-- an unknown IANA name falls back to a longitude-derived fixed offset instead of
-- silently pretending it is UTC.
local NAMED_ZONES = {
  ["AMERICA/TORONTO"] = { -300, "US" },
  ["AMERICA/MONTREAL"] = { -300, "US" },
  ["AMERICA/NEW_YORK"] = { -300, "US" },
  ["AMERICA/DETROIT"] = { -300, "US" },
  ["AMERICA/NASSAU"] = { -300, "US" },
  ["AMERICA/HALIFAX"] = { -240, "US" },
  ["AMERICA/GLACE_BAY"] = { -240, "US" },
  ["AMERICA/MONCTON"] = { -240, "US" },
  ["AMERICA/ST_JOHNS"] = { -210, "US" },
  ["AMERICA/CHICAGO"] = { -360, "US" },
  ["AMERICA/WINNIPEG"] = { -360, "US" },
  ["AMERICA/REGINA"] = { -360, nil },
  ["AMERICA/DENVER"] = { -420, "US" },
  ["AMERICA/EDMONTON"] = { -420, "US" },
  ["AMERICA/PHOENIX"] = { -420, nil },
  ["AMERICA/LOS_ANGELES"] = { -480, "US" },
  ["AMERICA/VANCOUVER"] = { -480, "US" },
  ["AMERICA/TIJUANA"] = { -480, "US" },
  ["AMERICA/ANCHORAGE"] = { -540, "US" },
  ["PACIFIC/HONOLULU"] = { -600, nil },
  ["AMERICA/MEXICO_CITY"] = { -360, nil },
  ["AMERICA/BOGOTA"] = { -300, nil },
  ["AMERICA/LIMA"] = { -300, nil },
  ["AMERICA/CARACAS"] = { -240, nil },
  ["AMERICA/SANTIAGO"] = { -240, "CHILE" },
  ["AMERICA/SAO_PAULO"] = { -180, nil },
  ["AMERICA/BUENOS_AIRES"] = { -180, nil },
  ["AMERICA/ARGENTINA/BUENOS_AIRES"] = { -180, nil },
  ["AMERICA/MONTEVIDEO"] = { -180, nil },

  ["EUROPE/LONDON"] = { 0, "EU" },
  ["EUROPE/DUBLIN"] = { 0, "EU" },
  ["EUROPE/LISBON"] = { 0, "EU" },
  ["ATLANTIC/CANARY"] = { 0, "EU" },
  ["EUROPE/PARIS"] = { 60, "EU" },
  ["EUROPE/BERLIN"] = { 60, "EU" },
  ["EUROPE/MADRID"] = { 60, "EU" },
  ["EUROPE/ROME"] = { 60, "EU" },
  ["EUROPE/AMSTERDAM"] = { 60, "EU" },
  ["EUROPE/BRUSSELS"] = { 60, "EU" },
  ["EUROPE/VIENNA"] = { 60, "EU" },
  ["EUROPE/PRAGUE"] = { 60, "EU" },
  ["EUROPE/WARSAW"] = { 60, "EU" },
  ["EUROPE/STOCKHOLM"] = { 60, "EU" },
  ["EUROPE/OSLO"] = { 60, "EU" },
  ["EUROPE/COPENHAGEN"] = { 60, "EU" },
  ["EUROPE/ZURICH"] = { 60, "EU" },
  ["EUROPE/BUDAPEST"] = { 60, "EU" },
  ["EUROPE/BELGRADE"] = { 60, "EU" },
  ["EUROPE/HELSINKI"] = { 120, "EU" },
  ["EUROPE/ATHENS"] = { 120, "EU" },
  ["EUROPE/BUCHAREST"] = { 120, "EU" },
  ["EUROPE/KYIV"] = { 120, "EU" },
  ["EUROPE/KIEV"] = { 120, "EU" },
  ["EUROPE/SOFIA"] = { 120, "EU" },
  ["EUROPE/TALLINN"] = { 120, "EU" },
  ["EUROPE/RIGA"] = { 120, "EU" },
  ["EUROPE/VILNIUS"] = { 120, "EU" },
  ["EUROPE/ISTANBUL"] = { 180, nil },
  ["EUROPE/MOSCOW"] = { 180, nil },

  ["AFRICA/ABIDJAN"] = { 0, nil },
  ["AFRICA/ACCRA"] = { 0, nil },
  ["AFRICA/CASABLANCA"] = { 60, nil },
  ["AFRICA/ALGIERS"] = { 60, nil },
  ["AFRICA/LAGOS"] = { 60, nil },
  ["AFRICA/CAIRO"] = { 120, nil },
  ["AFRICA/JOHANNESBURG"] = { 120, nil },
  ["AFRICA/HARARE"] = { 120, nil },
  ["AFRICA/MAPUTO"] = { 120, nil },
  ["AFRICA/NAIROBI"] = { 180, nil },
  ["AFRICA/ADDIS_ABABA"] = { 180, nil },

  ["ASIA/JERUSALEM"] = { 120, "ISRAEL" },
  ["ASIA/BEIRUT"] = { 120, "EU" },
  ["ASIA/DAMASCUS"] = { 180, nil },
  ["ASIA/RIYADH"] = { 180, nil },
  ["ASIA/BAGHDAD"] = { 180, nil },
  ["ASIA/KUWAIT"] = { 180, nil },
  ["ASIA/TEHRAN"] = { 210, nil },
  ["ASIA/DUBAI"] = { 240, nil },
  ["ASIA/KABUL"] = { 270, nil },
  ["ASIA/KARACHI"] = { 300, nil },
  ["ASIA/KOLKATA"] = { 330, nil },
  ["ASIA/CALCUTTA"] = { 330, nil },
  ["ASIA/KATHMANDU"] = { 345, nil },
  ["ASIA/DHAKA"] = { 360, nil },
  ["ASIA/YANGON"] = { 390, nil },
  ["ASIA/RANGOON"] = { 390, nil },
  ["ASIA/BANGKOK"] = { 420, nil },
  ["ASIA/JAKARTA"] = { 420, nil },
  ["ASIA/SINGAPORE"] = { 480, nil },
  ["ASIA/KUALA_LUMPUR"] = { 480, nil },
  ["ASIA/MANILA"] = { 480, nil },
  ["ASIA/HONG_KONG"] = { 480, nil },
  ["ASIA/SHANGHAI"] = { 480, nil },
  ["ASIA/TAIPEI"] = { 480, nil },
  ["ASIA/SEOUL"] = { 540, nil },
  ["ASIA/TOKYO"] = { 540, nil },

  ["AUSTRALIA/PERTH"] = { 480, nil },
  ["AUSTRALIA/DARWIN"] = { 570, nil },
  ["AUSTRALIA/ADELAIDE"] = { 570, "AU" },
  ["AUSTRALIA/BRISBANE"] = { 600, nil },
  ["AUSTRALIA/SYDNEY"] = { 600, "AU" },
  ["AUSTRALIA/MELBOURNE"] = { 600, "AU" },
  ["AUSTRALIA/HOBART"] = { 600, "AU" },
  ["PACIFIC/NOUMEA"] = { 660, nil },
  ["PACIFIC/AUCKLAND"] = { 720, "NZ" },
  ["PACIFIC/FIJI"] = { 720, nil },
  ["PACIFIC/CHATHAM"] = { 765, "NZ" },
  ["PACIFIC/TONGATAPU"] = { 780, nil },
}

local function parse_fixed_offset_minutes(time_zone_id)
  local raw = tostring(time_zone_id or "GMT"):gsub("%s+", "")
  local upper = raw:upper()
  if upper == "GMT" or upper == "UTC" or upper == "Z" then return 0 end

  local sign, hours = upper:match("^ETC/GMT([%+%-])(%d+)$")
  if sign and hours then
    local value = tonumber(hours)
    if value == nil or value > 14 then return nil end
    -- IANA Etc/GMT signs are intentionally reversed by POSIX convention.
    return sign == "+" and -value * 60 or value * 60
  end

  local minutes
  sign, hours, minutes = upper:match("^GMT([%+%-])(%d+):(%d%d)$")
  if not sign then sign, hours = upper:match("^GMT([%+%-])(%d+)$") end
  if not sign then sign, hours, minutes = upper:match("^UTC([%+%-])(%d+):(%d%d)$") end
  if not sign then sign, hours = upper:match("^UTC([%+%-])(%d+)$") end
  if not sign then sign, hours, minutes = upper:match("^([%+%-])(%d+):(%d%d)$") end
  if not sign then sign, hours = upper:match("^([%+%-])(%d+)$") end
  if not sign or not hours then return nil end

  local hour_value = tonumber(hours)
  local minute_value = tonumber(minutes) or 0
  if hour_value == nil or hour_value > 14 or minute_value > 59 then return nil end
  local total = hour_value * 60 + minute_value
  if total > 14 * 60 then return nil end
  return sign == "-" and -total or total
end

local function longitude_offset_minutes(longitude_deg)
  local lon = clamp(tonumber(longitude_deg) or 0.0, -180.0, 180.0)
  local zones
  if lon >= 0.0 then
    zones = math.floor(lon / 15.0 + 0.5)
  else
    zones = math.ceil(lon / 15.0 - 0.5)
  end
  local minutes = zones * 60
  return clamp(minutes, -12 * 60, 14 * 60)
end

local function resolve_zone(settings)
  local id = tostring(settings.time_zone_id or "GMT")
  local fixed = parse_fixed_offset_minutes(id)
  if fixed ~= nil then return fixed, nil, false end
  local named = NAMED_ZONES[id:upper()]
  if named then return named[1], named[2], false end
  return longitude_offset_minutes(settings.longitude), nil, true
end

local function date_after_or_equal(month, day, start_month, start_day)
  return month > start_month or (month == start_month and day >= start_day)
end

local function date_before(month, day, end_month, end_day)
  return month < end_month or (month == end_month and day < end_day)
end

local function dst_active_local(rule, base_offset_minutes, year, month, day, hour, minute)
  if rule == nil then return false end
  local clock = hour + minute / 60.0

  if rule == "US" then
    local start_day = nth_sunday(year, 3, 2)
    local end_day = nth_sunday(year, 11, 1)
    if month > 3 and month < 11 then return true end
    if month == 3 then return day > start_day or (day == start_day and clock >= 2.0) end
    if month == 11 then return day < end_day or (day == end_day and clock < 2.0) end
    return false
  end

  if rule == "EU" then
    local start_day = last_sunday(year, 3)
    local end_day = last_sunday(year, 10)
    local start_hour = 1.0 + base_offset_minutes / 60.0
    local end_hour = start_hour + 1.0
    if month > 3 and month < 10 then return true end
    if month == 3 then return day > start_day or (day == start_day and clock >= start_hour) end
    if month == 10 then return day < end_day or (day == end_day and clock < end_hour) end
    return false
  end

  if rule == "AU" then
    local start_day = nth_sunday(year, 10, 1)
    local end_day = nth_sunday(year, 4, 1)
    if month > 10 or month < 4 then return true end
    if month == 10 then return day > start_day or (day == start_day and clock >= 2.0) end
    if month == 4 then return day < end_day or (day == end_day and clock < 3.0) end
    return false
  end

  if rule == "NZ" then
    local start_day = last_sunday(year, 9)
    local end_day = nth_sunday(year, 4, 1)
    if month > 9 or month < 4 then return true end
    if month == 9 then return day > start_day or (day == start_day and clock >= 2.0) end
    if month == 4 then return day < end_day or (day == end_day and clock < 3.0) end
    return false
  end

  -- These two rules are intentionally conservative and cover the common modern
  -- schedule. Explicit GMT offsets remain available for historical edge cases.
  if rule == "CHILE" then
    local start_day = nth_sunday(year, 9, 1)
    local end_day = nth_sunday(year, 4, 1)
    if month > 9 or month < 4 then return true end
    if month == 9 then return day >= start_day end
    if month == 4 then return day < end_day end
    return false
  end

  if rule == "ISRAEL" then
    local start_day = last_sunday(year, 3) - 2 -- Friday before last Sunday.
    local end_day = last_sunday(year, 10)
    if month > 3 and month < 10 then return true end
    if month == 3 then return day > start_day or (day == start_day and clock >= 2.0) end
    if month == 10 then return day < end_day or (day == end_day and clock < 2.0) end
    return false
  end

  return false
end

local function effective_offset_minutes(settings, utc_millis)
  local base, rule, approximate = resolve_zone(settings)
  if not settings.use_dst or rule == nil then return base, approximate end

  -- First estimate local civil time using standard time. Then re-evaluate after
  -- applying DST, which handles transitions that cross a date boundary.
  local y, m, d, h, min = split_millis(utc_millis + base * MINUTE_MS)
  local active = dst_active_local(rule, base, y, m, d, h, min)
  local effective = base + (active and 60 or 0)
  if active then
    y, m, d, h, min = split_millis(utc_millis + effective * MINUTE_MS)
    active = dst_active_local(rule, base, y, m, d, h, min)
    effective = base + (active and 60 or 0)
  end
  return effective, approximate
end

local function julian_day(utc_millis)
  return utc_millis / DAY_MS + 2440587.5
end

-- Apparent geocentric solar right ascension and declination using the NOAA
-- low-order solar model. Accuracy is easily sufficient for rendering and is
-- substantially better than treating every noon as a zenith transit.
local function solar_ra_dec(utc_millis)
  local jd = julian_day(utc_millis)
  local t = (jd - 2451545.0) / 36525.0
  local mean_longitude = normalize_deg(280.46646 + t * (36000.76983 + t * 0.0003032))
  local mean_anomaly = normalize_deg(357.52911 + t * (35999.05029 - 0.0001537 * t)) * DEG
  local equation_center =
    math.sin(mean_anomaly) * (1.914602 - t * (0.004817 + 0.000014 * t)) +
    math.sin(2.0 * mean_anomaly) * (0.019993 - 0.000101 * t) +
    math.sin(3.0 * mean_anomaly) * 0.000289
  local true_longitude = mean_longitude + equation_center
  local omega = (125.04 - 1934.136 * t) * DEG
  local apparent_longitude = (true_longitude - 0.00569 - 0.00478 * math.sin(omega)) * DEG
  local mean_obliquity = 23.0 + (26.0 +
    (21.448 - t * (46.815 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0
  local obliquity = (mean_obliquity + 0.00256 * math.cos(omega)) * DEG
  local sin_lambda = math.sin(apparent_longitude)
  local right_ascension = math.atan(sin_lambda * math.cos(obliquity), math.cos(apparent_longitude))
  if right_ascension < 0.0 then right_ascension = right_ascension + TWO_PI end
  local declination = math.asin(clamp(math.sin(obliquity) * sin_lambda, -1.0, 1.0))
  return right_ascension, declination
end

local function local_sidereal_time_rad(utc_millis, longitude_deg)
  local jd = julian_day(utc_millis)
  local t = (jd - 2451545.0) / 36525.0
  local gmst = 280.46061837 + 360.98564736629 * (jd - 2451545.0) +
    0.000387933 * t * t - t * t * t / 38710000.0
  return normalize_deg(gmst + (longitude_deg or 0.0)) * DEG
end

local function apply_refraction(geometric_altitude_deg)
  local elevation = geometric_altitude_deg
  if elevation > 85.0 then return elevation end
  local tangent = math.tan(elevation * DEG)
  local correction_arcsec
  if elevation > 5.0 then
    correction_arcsec = 58.1 / tangent - 0.07 / (tangent * tangent * tangent) +
      0.000086 / (tangent * tangent * tangent * tangent * tangent)
  elseif elevation > -0.575 then
    correction_arcsec = 1735.0 + elevation * (-518.2 + elevation *
      (103.4 + elevation * (-12.79 + elevation * 0.711)))
  else
    correction_arcsec = -20.772 / tangent
  end
  return elevation + correction_arcsec / 3600.0
end

local function horizontal_from_ra_dec(ra, dec, utc_millis, latitude_deg, longitude_deg)
  local latitude = clamp(latitude_deg or 45.0, -90.0, 90.0) * DEG
  local hour_angle = normalize_rad(local_sidereal_time_rad(utc_millis, longitude_deg or 0.0) - ra)
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

function earth_time_solar.horizontal_from_ra_dec_hours(ra_hours, dec_deg, utc_millis, settings)
  local azimuth, altitude = horizontal_from_ra_dec(
    ra_hours * 15.0 * DEG,
    dec_deg * DEG,
    utc_millis,
    settings.latitude,
    settings.longitude)
  return azimuth, altitude
end

function earth_time_solar.color_cycle_phase_from_day_tick(tick)
  return normalize_tick(tick - 6000.0) / 24000.0
end

function earth_time_solar.parse_utc_offset_ms(time_zone_id)
  local minutes = parse_fixed_offset_minutes(time_zone_id)
  if minutes ~= nil then return minutes * MINUTE_MS end
  local named = NAMED_ZONES[tostring(time_zone_id or ""):upper()]
  return named and named[1] * MINUTE_MS or 0.0
end

function earth_time_solar.time_zone_info(settings, utc_millis)
  local offset, approximate = effective_offset_minutes(settings, utc_millis or minecraft.time.utc_millis())
  return {
    offset_minutes = offset,
    approximate = approximate,
  }
end

function earth_time_solar.resolve_observer_millis(settings, utc_millis)
  utc_millis = utc_millis or minecraft.time.utc_millis()
  if not settings.override_enabled or not (settings.simulate_date or settings.simulate_time) then
    return utc_millis
  end

  local offset_minutes = effective_offset_minutes(settings, utc_millis)
  local local_millis = utc_millis + offset_minutes * MINUTE_MS
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

  local base, rule = resolve_zone(settings)
  local target_offset = base
  if settings.use_dst and dst_active_local(rule, base, year, month, day, hour, minute) then
    target_offset = target_offset + 60
  end
  local target_local = days_from_civil(year, month, day) * DAY_MS +
    hour * HOUR_MS + minute * MINUTE_MS + second_ms
  return target_local - target_offset * MINUTE_MS
end

function earth_time_solar.sun_azimuth_altitude(utc_millis, settings)
  local right_ascension, declination = solar_ra_dec(utc_millis)
  local azimuth, altitude = horizontal_from_ra_dec(
    right_ascension, declination, utc_millis, settings.latitude, settings.longitude)
  return azimuth, altitude
end

function earth_time_solar.build_frame(settings, partial_ticks, utc_millis)
  local observer_millis = earth_time_solar.resolve_observer_millis(settings, utc_millis)
  local right_ascension, declination = solar_ra_dec(observer_millis)
  local sun_azimuth, sun_altitude, hour_angle, geometric_altitude = horizontal_from_ra_dec(
    right_ascension,
    declination,
    observer_millis,
    settings.latitude,
    settings.longitude)

  -- Minecraft phase remains a clock convention: 0 sunrise, .25 noon,
  -- .50 sunset, .75 midnight. It is intentionally separate from the actual
  -- physical zenith angle used to place the sun and drive shadows.
  local ha_unwrapped = hour_angle % TWO_PI
  if ha_unwrapped < 0.0 then ha_unwrapped = ha_unwrapped + TWO_PI end
  local day_tick = normalize_tick(6000.0 + ha_unwrapped / TWO_PI * 24000.0)

  -- The renderer reconstructs direction as:
  -- x = sin(angle) sin(yaw), y = cos(angle), z = sin(angle) cos(yaw).
  -- A zenith angle and north-clockwise azimuth therefore reproduce the actual
  -- horizontal solar coordinates instead of forcing the sun overhead at noon.
  local zenith_deg = clamp(90.0 - sun_altitude, 0.0, 180.0)
  local sun_angle = zenith_deg * DEG
  local moon_angle = (sun_angle + PI) % TWO_PI

  -- Canonical world-space direction from the observer toward the real sun.
  -- Azimuth is clockwise from north; +Y is up. Every renderer and shadow
  -- provider should consume this vector instead of independently rebuilding it.
  local altitude_rad = sun_altitude * DEG
  local azimuth_rad = sun_azimuth * DEG
  local horizontal = math.cos(altitude_rad)
  local sun_direction_x = horizontal * math.sin(azimuth_rad)
  local sun_direction_y = math.sin(altitude_rad)
  -- Minecraft's sky/world convention uses -Z for geographic north.
  -- Using +cos(azimuth) mirrors the sun north-to-south.
  local sun_direction_z = -horizontal * math.cos(azimuth_rad)

  -- Apparent solar time: 06:00 at tick 0, 12:00 at 6000, 18:00 at
  -- 12000 and 00:00 at 18000. This is based on hour angle, so the clock and
  -- the east/west position of the physical sun cannot drift apart.
  local solar_time_hours = (day_tick / 1000.0 + 6.0) % 24.0
  local zone = earth_time_solar.time_zone_info(settings, observer_millis)

  return {
    utc_millis = observer_millis,
    day_tick = day_tick,
    partial_ticks = partial_ticks or 0.0,
    celestial = day_tick / 24000.0,
    -- Preserve the engine convention used by the original working version:
    -- geographic azimuth 0° (north) maps to world yaw 180° (-Z).
    skydome_yaw_deg = normalize_signed_deg(180.0 - sun_azimuth),
    color_cycle_phase = earth_time_solar.color_cycle_phase_from_day_tick(day_tick),
    sun_angle = sun_angle,
    moon_angle = moon_angle,
    sun_azimuth_deg = sun_azimuth,
    sun_altitude_deg = sun_altitude,
    sun_geometric_altitude_deg = geometric_altitude,
    sun_direction_x = sun_direction_x,
    sun_direction_y = sun_direction_y,
    sun_direction_z = sun_direction_z,
    solar_hour_angle_rad = hour_angle,
    solar_time_hours = solar_time_hours,
    is_daylight = sun_altitude > -0.833,
    twilight = 1.0 - smoothstep(-18.0, 0.0, sun_altitude),
    time_zone_offset_minutes = zone.offset_minutes,
    time_zone_approximate = zone.approximate,
  }
end

return earth_time_solar
