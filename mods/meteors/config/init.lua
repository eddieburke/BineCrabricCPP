-- Meteors Configuration
local config = {}

config.settings = {
  gravity = { key = "gravity", label = "Gravity", kind = "slider", min = -0.12, max = -0.01, step = 0.005, decimals = 3, default = -0.04 },
  air_density = { key = "air_density", label = "Air Density", kind = "slider", min = 0.0002, max = 0.004, step = 0.0001, decimals = 4, default = 0.0012 },
  air_scale_height = { key = "air_scale_height", label = "Air Scale Height", kind = "slider", min = 10, max = 50, step = 1, default = 25.0 },
  stone_strength = { key = "stone_strength", label = "Fragmentation Strength", kind = "slider", min = 2, max = 20, step = 0.5, default = 8.0 },
  fragment_min_size = { key = "fragment_min_size", label = "Minimum Fragment Size", kind = "slider", min = 0.05, max = 0.5, step = 0.01, decimals = 2, default = 0.15 },
  trail_length = { key = "trail_length", label = "Trail Length", kind = "slider", min = 12, max = 96, integer = true, default = 54 },
  visual_quality = { key = "visual_quality", label = "Visual Quality", kind = "slider", min = 1, max = 3, integer = true, default = 3 },
  effect_density = { key = "effect_density", label = "Effect Density", kind = "slider", min = 1, max = 3, integer = true, default = 2 },
  max_meteors = { key = "max_meteors", label = "Max Meteors", kind = "slider", min = 10, max = 150, integer = true, default = 50 },
}

config.keybinds = {
  spawn_meteor = { default = minecraft.key_code("m"), label = "Spawn Meteor" },
  spawn_comet = { default = minecraft.key_code("n"), label = "Spawn Comet" },
}

config.model_version = "geo_stunning_4"
config.max_render_vertices = 48000
config.tau = math.pi * 2

function config.register()
  local settings_list = {}
  for _, v in pairs(config.settings) do
    settings_list[#settings_list + 1] = v
  end
  minecraft.settings.register("Meteors", settings_list)
  
  for name, data in pairs(config.keybinds) do
    minecraft.keybinds.register(name, data)
  end
end

function config.get(name)
  local setting = config.settings[name]
  if not setting then return nil end
  local value = minecraft.settings.get(setting.key)
  if value == nil then
    return setting.default
  end
  return value
end

return config
