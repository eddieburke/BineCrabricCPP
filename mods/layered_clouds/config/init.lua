-- Layered Clouds Configuration
local config = {}

config.CONFIG_FILE = "mod_LayeredClouds.cfg"
config.NATIVE_CLOUDS_FANCIER = 3

config.defaults = {
  layer_count = 6,
  base_opacity = 0.7,
  cloud_scale = 1.0,
  layer_height_spacing = 12.0,
  wind_speed = 1.0,
}

config.keys = {
  "layer_count",
  "base_opacity",
  "cloud_scale",
  "layer_height_spacing",
  "wind_speed",
}

config.names = {
  layer_count = "layerCount",
  base_opacity = "baseOpacity",
  cloud_scale = "cloudScale",
  layer_height_spacing = "layerHeightSpacing",
  wind_speed = "windSpeedMultiplier",
}

config.aliases = {}
for internal_name, file_name in pairs(config.names) do
  config.aliases[file_name] = internal_name
end

function config.load_config()
  local found
  local loaded_config, found = minecraft.config.load(config.CONFIG_FILE, config.defaults, {
    aliases = config.aliases,
  })
  
  -- Clamp values
  loaded_config.layer_count = math.floor(math.max(1, math.min(12, loaded_config.layer_count)))
  loaded_config.base_opacity = math.max(0.0, math.min(1.0, loaded_config.base_opacity))
  loaded_config.cloud_scale = math.max(0.1, math.min(4.0, loaded_config.cloud_scale))
  loaded_config.layer_height_spacing = math.max(1.0, math.min(64.0, loaded_config.layer_height_spacing))
  loaded_config.wind_speed = math.max(0.0, math.min(10.0, loaded_config.wind_speed))
  
  return loaded_config, found
end

function config.save_config(cfg)
  cfg.layer_count = math.floor(math.max(1, math.min(12, cfg.layer_count)))
  cfg.base_opacity = math.max(0.0, math.min(1.0, cfg.base_opacity))
  cfg.cloud_scale = math.max(0.1, math.min(4.0, cfg.cloud_scale))
  cfg.layer_height_spacing = math.max(1.0, math.min(64.0, cfg.layer_height_spacing))
  cfg.wind_speed = math.max(0.0, math.min(10.0, cfg.wind_speed))
  
  minecraft.config.save(config.CONFIG_FILE, cfg, {
    keys = config.keys,
    names = config.names,
  })
end

return config
