minecraft.settings.register("Fog Settings", {
  { key = "enabled", label = "Custom Fog", kind = "toggle", default = false },
  { key = "spherical", label = "Spherical Projection", kind = "toggle", default = true },
  { key = "start", label = "Fog Start", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.2 },
  { key = "exponential", label = "Exponential Fog", kind = "toggle", default = false },
  { key = "end", label = "Fog End", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.8 },
  { key = "density", label = "Fog Density", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0.1 },
  { key = "custom_color", label = "Custom Fog Color", kind = "toggle", default = false },
  { key = "red", label = "Fog Red", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0 },
  { key = "green", label = "Fog Green", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0 },
  { key = "blue", label = "Fog Blue", kind = "slider", min = 0, max = 1, step = 0.01, decimals = 2, default = 0 },
})

minecraft.on(minecraft.events.fog_settings, {}, function(event)
  event.enabled = minecraft.settings.get("enabled") or false
  event.spherical = minecraft.settings.get("spherical") ~= false
  event.start = minecraft.settings.get("start") or 0.2
  event.exponential = minecraft.settings.get("exponential") or false
  event["end"] = minecraft.settings.get("end") or 0.8
  event.density = minecraft.settings.get("density") or 0.1
  event.custom_color = minecraft.settings.get("custom_color") or false
  event.red = minecraft.settings.get("red") or 0
  event.green = minecraft.settings.get("green") or 0
  event.blue = minecraft.settings.get("blue") or 0
  return event
end)
