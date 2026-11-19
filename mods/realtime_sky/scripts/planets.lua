local solar = require("scripts.solar")

local planets = {}

local DEG = math.pi / 180.0
local TWO_PI = math.pi * 2.0
local clamp = minecraft.util.clamp

local function smoothstep(edge0, edge1, value)
  if edge0 == edge1 then return value < edge0 and 0.0 or 1.0 end
  local t = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)
end

-- JPL Standish (1992) Table 1 J2000 elements (1800-2050 AD).
local EARTH_EL = {
  1.00000261, 0.00000562, 0.01671123, -0.00004392, -0.00001531, -0.01294668,
  100.46457166, 35999.37244981, 102.93768193, 0.32327364, 0.0, 0.0, 0, 0, 0, 0,
}

local BODIES = {
  {
    id = "mercury",
    setting = "planet_mercury",
    half_size = 0.38,
    day_fade_start = -1.0,
    day_fade_end = -8.0,
    el = {
      0.38709927, 0.00000037, 0.20563593, 0.00001906, 7.00497902, -0.00594749,
      252.25032350, 149472.67411175, 77.45779628, 0.16047689, 48.33076593, -0.12534081,
      0, 0, 0, 0,
    },
  },
  {
    id = "venus",
    setting = "planet_venus",
    half_size = 0.56,
    day_fade_start = 6.0,
    day_fade_end = -4.0,
    el = {
      0.72333566, 0.00000390, 0.00677672, -0.00004107, 3.39467605, -0.00078890,
      181.97909950, 58517.81538729, 131.60246718, 0.00268329, 76.67984255, -0.27769418,
      0, 0, 0, 0,
    },
  },
  {
    id = "mars",
    setting = "planet_mars",
    half_size = 0.50,
    day_fade_start = 1.0,
    day_fade_end = -6.0,
    el = {
      1.52371034, 0.00001847, 0.09339410, 0.00007882, 1.84969142, -0.00813131,
      -4.55343205, 19140.30268499, -23.94362959, 0.44441088, 49.55953891, -0.29257343,
      0, 0, 0, 0,
    },
  },
  {
    id = "jupiter",
    setting = "planet_jupiter",
    half_size = 0.84,
    day_fade_start = 4.0,
    day_fade_end = -5.0,
    el = {
      5.20288700, -0.00011607, 0.04838624, -0.00013253, 1.30439695, -0.00183714,
      34.39644051, 3034.74612775, 14.72847983, 0.21252668, 100.47390909, 0.20469106,
      -0.00012452, 0.06064060, -0.35635438, 38.35125000,
    },
  },
  {
    id = "saturn",
    setting = "planet_saturn",
    half_size = 0.78,
    day_fade_start = 1.5,
    day_fade_end = -6.5,
    el = {
      9.53667594, -0.00125060, 0.05386179, -0.00050991, 2.48599187, 0.00193609,
      49.95424423, 1222.49362201, 92.59887831, -0.41897216, 113.66242448, -0.28867794,
      0.00025899, -0.13434469, 0.87320147, 38.35125000,
    },
  },
  {
    id = "uranus",
    setting = "planet_uranus",
    half_size = 0.48,
    day_fade_start = -2.5,
    day_fade_end = -10.5,
    el = {
      19.18916464, -0.00196176, 0.04725744, -0.00004397, 0.77263783, -0.00242939,
      313.23810451, 428.48202785, 170.95427630, 0.40805281, 74.01692503, 0.04240589,
      0.00058331, -0.97731848, 0.17689245, 7.67025000,
    },
  },
  {
    id = "neptune",
    setting = "planet_neptune",
    half_size = 0.46,
    day_fade_start = -3.0,
    day_fade_end = -11.0,
    el = {
      30.06992276, 0.00026291, 0.00859048, 0.00005105, 1.77004347, 0.00035372,
      -55.12002969, 218.45945325, 44.96476227, -0.32241464, 131.78422574, -0.00508664,
      -0.00041348, 0.68346318, -0.10162547, 7.67025000,
    },
  },
  {
    id = "pluto",
    setting = "planet_pluto",
    half_size = 0.36,
    day_fade_start = -4.0,
    day_fade_end = -12.0,
    el = {
      39.48211675, -0.00031596, 0.24882730, 0.00005170, 17.14001206, 0.00004818,
      238.92903833, 145.20780515, 224.06891629, -0.04062942, 110.30393684, -0.01183482,
      -0.01262724, 0, 0, 0,
    },
  },
}

local function helio_ecliptic(el, t)
  local a = el[1] + el[2] * t
  local e = el[3] + el[4] * t
  local I = (el[5] + el[6] * t) * DEG
  local L = el[7] + el[8] * t
  local wbar = el[9] + el[10] * t
  local O = (el[11] + el[12] * t) * DEG
  local w = (wbar * DEG) - O

  local f_arg = el[16] * t * DEG
  local M_deg = (L - wbar) + el[13] * t * t + el[14] * math.cos(f_arg) + el[15] * math.sin(f_arg)
  local M = ((M_deg + 180.0) % 360.0 - 180.0) * DEG

  local E = M + e * math.sin(M)
  for _ = 1, 6 do
    local denom = 1.0 - e * math.cos(E)
    if math.abs(denom) < 1.0e-12 then break end
    E = E - (E - e * math.sin(E) - M) / denom
  end

  local xo = a * (math.cos(E) - e)
  local yo = a * math.sqrt(math.max(0.0, 1.0 - e * e)) * math.sin(E)

  local cw, sw = math.cos(w), math.sin(w)
  local cO, sO = math.cos(O), math.sin(O)
  local cI, sI = math.cos(I), math.sin(I)

  local x = (cw * cO - sw * sO * cI) * xo + (-sw * cO - cw * sO * cI) * yo
  local y = (cw * sO + sw * cO * cI) * xo + (-sw * sO + cw * cO * cI) * yo
  local z = (sw * sI) * xo + (cw * sI) * yo
  return x, y, z
end

local eq_cache_key
local eq_cache = {}
for i = 1, #BODIES do
  eq_cache[i] = { ra = 0.0, dec = 0.0, illumination = 1.0 }
end

local function update_equatorial(utc_millis)
  local key = math.floor(utc_millis / 1000.0)
  if eq_cache_key == key then
    return eq_cache
  end
  local jd = solar.julian_day(utc_millis)
  local t = solar.julian_centuries(jd)
  local eps = solar.true_equator(t)
  local ce, se = math.cos(eps), math.sin(eps)
  local xe, ye, ze = helio_ecliptic(EARTH_EL, t)

  for i = 1, #BODIES do
    local x, y, z = helio_ecliptic(BODIES[i].el, t)
    local gx, gy, gz = x - xe, y - ye, z - ze
    local dist_g = math.sqrt(gx * gx + gy * gy + gz * gz)
    local dist_h = math.sqrt(x * x + y * y + z * z)

    local xeq = gx
    local yeq = gy * ce - gz * se
    local zeq = gy * se + gz * ce
    local ra = math.atan(yeq, xeq)
    if ra < 0.0 then ra = ra + TWO_PI end
    local dec = math.asin(clamp(zeq / math.max(dist_g, 1.0e-12), -1.0, 1.0))

    local cos_phase = clamp((x * gx + y * gy + z * gz) / math.max(dist_h * dist_g, 1.0e-12), -1.0, 1.0)
    local slot = eq_cache[i]
    slot.ra = ra
    slot.dec = dec
    slot.illumination = (1.0 + cos_phase) * 0.5
  end

  eq_cache_key = key
  return eq_cache
end

local visible_scratch = {}
local visible_entries = {}
for i = 1, #BODIES do
  visible_entries[i] = {}
end

function planets.collect(settings, frame, weather_alpha, sky_darkness_override)
  local count = 0
  if settings.planets_enabled ~= true or weather_alpha <= 0.001 then
    return visible_scratch, 0
  end

  local eq = update_equatorial(frame.utc_millis)
  local sidereal = solar.local_sidereal_time_rad(frame.utc_millis, settings.longitude)
  local sdx = frame.sun_direction_x
  local sdy = frame.sun_direction_y
  local sdz = frame.sun_direction_z
  local sun_alt = frame.sun_altitude_deg or -90.0
  local daylight_mode = settings.planet_daylight == true

  for i = 1, #BODIES do
    local spec = BODIES[i]
    if settings[spec.setting] ~= false then
      local pos = eq[i]
      local az, alt = solar.horizontal_from_sidereal(sidereal, pos.ra, pos.dec, settings.latitude)
      if alt > -1.0 then
        local dx, dy, dz = solar.direction_from_azimuth_altitude(az, alt)
        local horizon_fade = smoothstep(-0.5, 2.5, alt)
        local sun_dot = dx * sdx + dy * sdy + dz * sdz
        local glare_fade = daylight_mode and 1.0 or (1.0 - smoothstep(0.996, 0.9992, sun_dot))
        local sky_fade
        if daylight_mode then
          sky_fade = 1.0
        elseif sky_darkness_override ~= nil then
          sky_fade = sky_darkness_override
        else
          sky_fade = smoothstep(spec.day_fade_start, spec.day_fade_end, sun_alt)
        end
        local alpha = weather_alpha * horizon_fade * glare_fade * sky_fade
        if alpha > 0.01 then
          count = count + 1
          local entry = visible_entries[count]
          entry.id = spec.id
          entry.half_size = spec.half_size
          entry.dx = dx
          entry.dy = dy
          entry.dz = dz
          entry.illumination = pos.illumination
          entry.alpha = alpha
          visible_scratch[count] = entry
        end
      end
    end
  end

  return visible_scratch, count
end

return planets
