-- Realtime Sky Configuration
-- Centralized settings and constants

local config = {}

-- Default settings
config.DEFAULTS = {
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

-- Settings keys for serialization
config.SETTINGS_KEYS = {
  "enabled", "time_zone_id", "latitude", "longitude", "use_dst", "drive_sun", 
  "show_simulate_panel", "override_enabled", "simulate_date", "simulate_time",
  "sim_year", "sim_month", "sim_day", "sim_hour", "sim_minute",
}

-- File name mappings
config.SETTINGS_NAMES = {
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

-- Build aliases lookup table
config.SETTINGS_ALIASES = {}
for internal_name, file_name in pairs(config.SETTINGS_NAMES) do
  config.SETTINGS_ALIASES[file_name] = internal_name
end

-- Constants
config.SKY_DOME_RADIUS = 80.0
config.SKY_BODY_RADIUS = 75.0
config.CELESTIAL_GRID = 16
config.SUN_HALF_SIZE = 3.65
config.SUN_TEXTURE_CYCLE_MS = 18000.0
config.SUN_PULSE_CYCLE_MS = 8000.0
config.SUN_PULSE_AMOUNT = 0.010
config.MOON_MEAN_HALF_SIZE = 2.00
config.TWO_PI = math.pi * 2.0

return config
