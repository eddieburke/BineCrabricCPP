-- ================================================================
-- MOD: fog_settings
-- Clean, separated logic with standardized config access
-- ================================================================

local config = require("fog_settings.config")

minecraft.on(minecraft.events.fog_settings, {}, function(event)
  event.enabled = config.enabled
  event.spherical = config.spherical
  event.start = config.start
  event.exponential = config.exponential
  event["end"] = config.end_val
  event.density = config.density
  event.custom_color = config.custom_color
  event.red = config.red
  event.green = config.green
  event.blue = config.blue
  return event
end)
