-- Camera Configuration
local config = {}

config.defaults = {
  fov = 70.0,
  smoothness = 0.1,
  auto_rotate = false,
  rotate_speed = 5.0,
}

config.keys = {
  "fov",
  "smoothness",
  "auto_rotate",
  "rotate_speed",
}

function config.load()
  local loaded_config, found = minecraft.config.load("mod_camera.cfg", config.defaults)
  
  -- Clamp values
  loaded_config.fov = math.max(30.0, math.min(120.0, loaded_config.fov))
  loaded_config.smoothness = math.max(0.01, math.min(1.0, loaded_config.smoothness))
  loaded_config.rotate_speed = math.max(0.1, math.min(30.0, loaded_config.rotate_speed))
  
  return loaded_config, found
end

function config.save(cfg)
  cfg.fov = math.max(30.0, math.min(120.0, cfg.fov))
  cfg.smoothness = math.max(0.01, math.min(1.0, cfg.smoothness))
  cfg.rotate_speed = math.max(0.1, math.min(30.0, cfg.rotate_speed))
  
  minecraft.config.save("mod_camera.cfg", cfg, { keys = config.keys })
end

return config
