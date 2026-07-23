-- Astronomy Calculator Module
-- Celestial mechanics and position calculations

local config = require("realtime_sky.config.init")
local astronomy = {}

local TWO_PI = config.TWO_PI
local DEG_TO_RAD = math.pi / 180.0
local RAD_TO_DEG = 180.0 / math.pi

-- Normalize angle to [-pi, pi]
local function normalize_angle(angle)
  while angle > math.pi do angle = angle - TWO_PI end
  while angle < -math.pi do angle = angle + TWO_PI end
  return angle
end

-- Calculate Julian Day from calendar date
function astronomy.julian_day(year, month, day, hour, minute)
  if month <= 2 then
    year = year - 1
    month = month + 12
  end
  
  local A = math.floor(year / 100)
  local B = 2 - A + math.floor(A / 4)
  
  local JD = math.floor(365.25 * (year + 4716)) + 
             math.floor(30.6001 * (month + 1)) + 
             day + B - 1524.5
  
  -- Add time fraction
  JD = JD + (hour + minute / 60.0) / 24.0
  
  return JD
end

-- Calculate Julian Century from JD
function astronomy.julian_century(JD)
  return (JD - 2451545.0) / 36525.0
end

-- Calculate sun position (simplified)
function astronomy.sun_position(JD)
  local T = astronomy.julian_century(JD)
  
  -- Mean longitude of the Sun
  local L0 = (280.46646 + 36000.76983 * T + 0.000303 * T * T) % 360
  
  -- Mean anomaly of the Sun
  local M = (357.52911 + 35999.05029 * T - 0.0001537 * T * T) % 360
  local M_rad = M * DEG_TO_RAD
  
  -- Equation of center
  local C = (1.914602 - 0.004817 * T - 0.000014 * T * T) * math.sin(M_rad) +
            (0.019993 - 0.000101 * T) * math.sin(2 * M_rad) +
            0.000289 * math.sin(3 * M_rad)
  
  -- Sun's true longitude
  local sun_lon = L0 + C
  
  -- Obliquity of ecliptic
  local epsilon = 23.439 - 0.0130042 * T
  
  -- Convert to radians
  local lon_rad = sun_lon * DEG_TO_RAD
  local eps_rad = epsilon * DEG_TO_RAD
  
  -- Calculate right ascension and declination
  local sin_ra = math.cos(eps_rad) * math.sin(lon_rad)
  local cos_ra = math.cos(lon_rad)
  local ra = math.atan2(sin_ra, cos_ra)
  
  local sin_dec = math.sin(eps_rad) * math.sin(lon_rad)
  local dec = math.asin(sin_dec)
  
  return {
    right_ascension = ra,
    declination = dec,
    longitude = sun_lon
  }
end

-- Calculate moon position (simplified)
function astronomy.moon_position(JD)
  local T = astronomy.julian_century(JD)
  
  -- Moon's mean longitude
  local L_prime = (218.3164477 + 481267.88123421 * T - 0.0015786 * T * T) % 360
  
  -- Moon's mean anomaly
  local M_prime = (134.9633964 + 477198.8675055 * T + 0.0087414 * T * T + 
                   0.00001549 * T * T * T) % 360
  local M_prime_rad = M_prime * DEG_TO_RAD
  
  -- Moon's mean distance
  local F = (93.2720950 + 483202.0175233 * T - 0.0036539 * T * T - 
             0.00000340 * T * T * T) % 360
  
  -- Moon's longitude correction (simplified)
  local D = (297.8501921 + 445267.1114034 * T - 0.0018819 * T * T + 
             0.000005458 * T * T * T) % 360
  
  -- Longitude perturbation terms (major ones only)
  local lon_corr = 6.289 * math.sin(M_prime_rad) -
                   1.274 * math.sin((2 * D - M_prime) * DEG_TO_RAD) -
                   0.658 * math.sin(2 * D * DEG_TO_RAD)
  
  local moon_lon = L_prime + lon_corr
  
  -- Latitude (simplified)
  local moon_lat = 5.128 * math.sin(F * DEG_TO_RAD)
  
  return {
    longitude = moon_lon,
    latitude = moon_lat,
    phase_angle = D
  }
end

-- Convert equatorial to horizontal coordinates
function astronomy.equatorial_to_horizontal(ra, dec, lat, lon, JD)
  -- Calculate Local Sidereal Time
  local T = astronomy.julian_century(JD)
  local GMST = 280.46061837 + 360.98564736629 * (JD - 2451545) +
               0.000387933 * T * T - T * T * T / 38710000
  GMST = GMST % 360
  
  -- Local Hour Angle
  local LST = GMST + lon
  local HA = LST - (ra * RAD_TO_DEG)
  local HA_rad = HA * DEG_TO_RAD
  local dec_rad = dec
  local lat_rad = lat * DEG_TO_RAD
  
  -- Horizontal coordinates
  local sin_alt = math.sin(dec_rad) * math.sin(lat_rad) +
                  math.cos(dec_rad) * math.cos(lat_rad) * math.cos(HA_rad)
  local alt = math.asin(sin_alt)
  
  local cos_az = (math.sin(dec_rad) - math.sin(alt) * math.sin(lat_rad)) /
                 (math.cos(alt) * math.cos(lat_rad))
  cos_az = math.max(-1, math.min(1, cos_az))
  local az = math.acos(cos_az)
  
  if math.sin(HA_rad) > 0 then
    az = TWO_PI - az
  end
  
  return {
    altitude = alt,
    azimuth = az
  }
end

return astronomy
