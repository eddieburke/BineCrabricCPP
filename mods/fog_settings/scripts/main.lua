local config = require("config")

minecraft.on("fog_settings", {}, function(event)
  if event.shader_pack_active then return event end
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
