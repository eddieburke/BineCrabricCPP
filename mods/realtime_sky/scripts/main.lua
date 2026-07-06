local earth_time_solar = minecraft.require("scripts.earth_time_solar")

local SCREEN_ID = "realtime_sky:settings"
local CONFIG_FILE = "realtime_sky.txt"
local SKY_PROVIDER_PRIORITY = 50

local SETTINGS_DEFAULTS = {
  enabled = false,
  latitude = 45.0,
  longitude = 0.0,
}

local SETTINGS_KEYS = { "enabled", "latitude", "longitude" }
local settings = minecraft.util.copy(SETTINGS_DEFAULTS)

local function save_settings()
  minecraft.config.save(CONFIG_FILE, settings, {
    keys = SETTINGS_KEYS,
    separator = ":",
  })
end

local function load_settings()
  local found
  settings, found = minecraft.config.load(CONFIG_FILE, SETTINGS_DEFAULTS)
  if not found then
    save_settings()
  end
end

local function utc_millis()
  return minecraft.time.utc_millis()
end

local clamp = minecraft.util.clamp

load_settings()

minecraft.on(minecraft.events.client_tick, {
  before = false,
  paused = false,
  has_world = true,
  priority = SKY_PROVIDER_PRIORITY,
  when = function()
    return settings.enabled
  end,
}, function(event)
  local frame = earth_time_solar.build_frame(settings, 0.0, utc_millis())
  local current = event.world_time or 0
  local day_base = current - (current % 24000)
  minecraft.world.set_time(day_base + frame.day_tick)
end)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.sky,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = SKY_PROVIDER_PRIORITY,
  when = function()
    return settings.enabled
  end,
}, function(event)
  local frame = earth_time_solar.build_frame(settings, event.tick_delta or 0.0, utc_millis())
  event.celestial_angle = frame.sun_angle
  event.sky_yaw_deg = frame.skydome_yaw_deg
  event.astronomy_enabled = true
  event.astronomy_utc_millis = frame.utc_millis
  event.observer_latitude_deg = settings.latitude
  event.observer_longitude_deg = settings.longitude
end)

local function clamp_settings()
  settings.latitude = clamp(settings.latitude, -90.0, 90.0)
  settings.longitude = clamp(settings.longitude, -180.0, 180.0)
end

clamp_settings()
minecraft.screen.settings({
  id = SCREEN_ID,
  title = "Real Time Sky Settings",
  parent_screen = minecraft.screen.ids.detail_settings,
  parent_region = minecraft.screen.regions.footer,
  button_label = "Real Time Settings...",
  values = function() return settings end,
  sliders = {
    { key = "latitude", min = -90, max = 90,
      format = function(v) return string.format("Lat: %.1f", v) end },
    { key = "longitude", min = -180, max = 180,
      format = function(v) return string.format("Lon: %.1f", v) end },
  },
  toggles = {
    { key = "enabled", label = "Enabled" },
  },
  on_change = clamp_settings,
  on_save = save_settings,
})

minecraft.log("info", "realtime_sky loaded")
