local config = {}

config.settings = {
  { key = "gravity", label = "Gravity", kind = "slider", min = -0.06, max = -0.002, step = 0.002, decimals = 3, default = -0.02 },
  { key = "air_density", label = "Air Density", kind = "slider", min = 0.0002, max = 0.004, step = 0.0001, decimals = 4, default = 0.0012 },
  { key = "air_scale_height", label = "Air Scale Height", kind = "slider", min = 10, max = 50, step = 1, default = 25.0 },
  { key = "stone_strength", label = "Fragmentation Strength", kind = "slider", min = 2, max = 20, step = 0.5, decimals = 1, default = 8.0 },
  { key = "fragment_min_size", label = "Minimum Fragment Size", kind = "slider", min = 0.05, max = 0.5, step = 0.01, decimals = 2, default = 0.15 },
  { key = "trail_length", label = "Trail Length", kind = "slider", min = 12, max = 96, integer = true, default = 54 },
  { key = "visual_quality", label = "Visual Quality", kind = "slider", min = 1, max = 3, integer = true, default = 3 },
  { key = "effect_density", label = "Effect Density", kind = "slider", min = 1, max = 3, integer = true, default = 2 },
  { key = "max_meteors", label = "Max Meteors", kind = "slider", min = 10, max = 150, integer = true, default = 50 },
}

config.keybinds = {
  spawn_meteor = { default = minecraft.key_code("m"), label = "Spawn Meteor" },
  spawn_comet = { default = minecraft.key_code("n"), label = "Spawn Comet" },
}

config.max_render_vertices = 48000

local byKey = {}
for _, setting in ipairs(config.settings) do
  byKey[setting.key] = setting
end

function config.register()
  minecraft.settings.register("Meteors", config.settings)
  for name, data in pairs(config.keybinds) do
    minecraft.keybinds.register(name, data)
  end
end

function config.get(name)
  local setting = byKey[name]
  if not setting then return nil end
  local value = minecraft.settings.get("meteors." .. setting.key)
  if value == nil then
    return setting.default
  end
  return value
end

return config
